import OBSWebSocket, {EventSubscription, OBSEventTypes, OBSResponseTypes} from 'obs-websocket-js';
import dotenv from 'dotenv';
import WebSocket from 'ws';

export interface ObsAudioSource {
    name: string;
    volume: number;
    muted: boolean;
}

export enum CaptureState {
    UNKNOWN = 0,
    IDLE = 1,
    RECORDING = 2,
    STREAMING = 3,
    RECORDING_AND_STREAMING = 4,
    RECORDING_PAUSED = 5,
}

export interface ObsState {
    is_muted?: boolean;
    currentScene?: string;
    recording_time?: string;
    streaming_time?: string;
    captureState?: CaptureState;
    audioSources?: ObsAudioSource[];
    connected: boolean;
}


dotenv.config();

const connectedClients: WebSocket[] = [];
let obsState: ObsState = {
    connected: false,
};

const {
    SERVER_PORT,
    OBS_SERVER_ADDRESS,
    OBS_SERVER_PORT,
    OBS_SERVER_PASSWORD,
    MICROPHONE_NAME,
} = process.env;

const obs = new OBSWebSocket();

const wss = new WebSocket.Server({port: SERVER_PORT ? parseInt(SERVER_PORT) : 8080});

function getCaptureState(streamResponse: OBSResponseTypes['GetStreamStatus'], recordResponse: OBSResponseTypes['GetRecordStatus']): CaptureState {
    if (streamResponse.outputActive) {
        if (recordResponse.outputActive) {
            return CaptureState.RECORDING_AND_STREAMING;
        } else {
            return CaptureState.STREAMING;
        }
    } else {
        if (recordResponse.outputPaused) {
            return CaptureState.RECORDING_PAUSED;
        } else {
            if (recordResponse.outputActive) {
                return CaptureState.RECORDING;
            } else {
                return CaptureState.IDLE;
            }
        }
    }
}

function outputStateToCaptureState(recording: boolean, outputState: OBSEventTypes['StreamStateChanged']['outputState'] | OBSEventTypes['RecordStateChanged']['outputState']): CaptureState {
    switch (outputState) {
    case 'OBS_WEBSOCKET_OUTPUT_STARTING':
    case 'OBS_WEBSOCKET_OUTPUT_STARTED':
        return recording ? CaptureState.RECORDING : CaptureState.STREAMING;
    case 'OBS_WEBSOCKET_OUTPUT_STOPPING':
    case 'OBS_WEBSOCKET_OUTPUT_STOPPED':
        return CaptureState.IDLE;
    case 'OBS_WEBSOCKET_OUTPUT_RECONNECTING':
    case 'OBS_WEBSOCKET_OUTPUT_RECONNECTED':
        return recording ? CaptureState.RECORDING : CaptureState.STREAMING;
    case 'OBS_WEBSOCKET_OUTPUT_PAUSED':
        return CaptureState.RECORDING_PAUSED;
    case 'OBS_WEBSOCKET_OUTPUT_RESUMED':
        return recording ? CaptureState.RECORDING : CaptureState.STREAMING;
    case 'OBS_WEBSOCKET_OUTPUT_UNKNOWN':
    default:
        return CaptureState.UNKNOWN;
    }
}

async function obs_reconnect() {
    try {
        await obs.connect(`ws://${OBS_SERVER_ADDRESS}:${OBS_SERVER_PORT}`, OBS_SERVER_PASSWORD, {
            eventSubscriptions: EventSubscription.All,
        });
    } catch (e) {
        obsState = {
            connected: false,
        };
        for (const client of connectedClients) {
            sendFullStatus(client);
        }
    }
}

async function updateFullStatus() {
    if (!obsState.connected) {
        await obs_reconnect();
        return;
    }

    const responses = await obs.callBatch([
        {
            requestType: 'GetStreamStatus',
        },
        {
            requestType: 'GetRecordStatus',
        },
        {
            requestType: 'GetInputMute',
            requestData: {
                inputName: MICROPHONE_NAME as string,
            },
        },
        {
            requestType: 'GetCurrentProgramScene',
        }
    ]);

    const [streamStatus, recordStatus, muteStatus, sceneStatus] = responses;
    const streamResponse = streamStatus.responseData as OBSResponseTypes['GetStreamStatus'];
    const recordResponse = recordStatus.responseData as OBSResponseTypes['GetRecordStatus'];
    const monitorResponse = muteStatus.responseData as OBSResponseTypes['GetInputMute'];
    const sceneResponse = sceneStatus.responseData as OBSResponseTypes['GetCurrentProgramScene'];

    obsState.streaming_time = streamResponse.outputTimecode;
    obsState.recording_time = recordResponse.outputTimecode;
    obsState.captureState = getCaptureState(streamResponse, recordResponse);
    obsState.currentScene = sceneResponse.currentProgramSceneName;
    obsState.is_muted = monitorResponse.inputMuted;

    if (obsState.captureState === CaptureState.RECORDING || obsState.captureState === CaptureState.RECORDING_AND_STREAMING) {
        for (const client of connectedClients) {
            client.send(JSON.stringify({
                type: 'singleStatus',
                key: 'recording_time',
                value: obsState.recording_time,
            }));
        }
    }

    if (obsState.captureState === CaptureState.STREAMING || obsState.captureState === CaptureState.RECORDING_AND_STREAMING) {
        for (const client of connectedClients) {
            client.send(JSON.stringify({
                type: 'singleStatus',
                key: 'streaming_time',
                value: obsState.streaming_time,
            }));
        }
    }
}

function sendFullStatus(ws: WebSocket) {
    const msg = JSON.stringify({
        type: 'fullStatus',
        ...obsState,
    });
    //console.log('Sending full status', msg);
    ws.send(msg);
}

wss.on('connection', async (ws) => {
    connectedClients.push(ws);

    console.log(`New client connected. Total clients: ${connectedClients.length}`);

    ws.on('message', async (message) => {
        let parsed: never | null = null;
        try {
            parsed = JSON.parse(message as never);
        } catch (e) {
            console.log('Error parsing message', e);
        }

        if (!parsed)
            return;

        const {type} = parsed;

        switch (type) {
        case 'fullStatus': {
            // send obsState
            if (typeof obsState.is_muted === 'undefined') {
                await updateFullStatus();
            }
            sendFullStatus(ws);
            break;
        }
        }
    });

    ws.on('close', () => {
        connectedClients.splice(connectedClients.indexOf(ws), 1);
        console.log(`Client disconnected. Total clients: ${connectedClients.length}`);
    });

    ws.on('error', (error) => {
        console.log('Error', error);
        connectedClients.splice(connectedClients.indexOf(ws), 1);
    });

    ws.on('unexpected-response', (request, response) => {
        console.log('Unexpected response', request, response);
    });
});

wss.on('error', (error) => {
    console.log('Error', error);

    obsState.connected = false;
});

wss.on('close', async () => {
    console.log('Closed');

    obsState.connected = false;

    await obs.disconnect();
});

// obs log all events
obs.on('InputMuteStateChanged', (event: OBSEventTypes['InputMuteStateChanged']) => {
    console.log('InputMuteStateChanged', event);

    if (event.inputName === MICROPHONE_NAME) {
        obsState.is_muted = event.inputMuted;

        connectedClients.forEach((client) => {
            client.send(JSON.stringify({
                type: 'singleStatus',
                key: 'is_muted',
                value: obsState.is_muted,
            }));
        });
    }
});

obs.on('RecordStateChanged', (event: OBSEventTypes['RecordStateChanged']) => {
    console.log('RecordStateChanged', event);

    obsState.captureState = outputStateToCaptureState(true, event.outputState);

    connectedClients.forEach((client) => {
        client.send(JSON.stringify({
            type: 'singleStatus',
            key: 'captureState',
            value: obsState.captureState,
        }));
    });
});

obs.on('StreamStateChanged', (event: OBSEventTypes['StreamStateChanged']) => {
    console.log('StreamStateChanged', event);

    obsState.captureState = outputStateToCaptureState(false, event.outputState);

    connectedClients.forEach((client) => {
        client.send(JSON.stringify({
            type: 'singleStatus',
            key: 'captureState',
            value: obsState.captureState,
        }));
    });
});

obs.on('CurrentProgramSceneChanged', (event: OBSEventTypes['CurrentProgramSceneChanged']) => {
    console.log('CurrentProgramSceneChanged', event);

    obsState.currentScene = event.sceneName;

    connectedClients.forEach((client) => {
        client.send(JSON.stringify({
            type: 'singleStatus',
            key: 'currentScene',
            value: obsState.currentScene,
        }));
    });
});

obs.on('ConnectionClosed', (e) => {
    console.log('ConnectionClosed', e);

    obsState.connected = false;
});

obs.on('ConnectionOpened', async () => {
    console.log('ConnectionOpened');

    obsState.connected = true;
});

obs.on('ConnectionError', (e) => {
    console.log('AuthenticationSuccess', e);
});

setInterval(async () => {
    await updateFullStatus();
}, 1000);

obs_reconnect();

console.log('Server started');
