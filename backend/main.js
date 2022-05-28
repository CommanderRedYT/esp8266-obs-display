import { WebSocketServer } from 'ws';
import OBSWebSocket from 'obs-websocket-js';
import dotenv from 'dotenv';

dotenv.config();

const wss = new WebSocketServer({ port: process.env.SERVER_PORT });
const obs = new OBSWebSocket();

const obs_address = `${process.env.OBS_SERVER_ADDRESS}:${process.env.OBS_SERVER_PORT}`;

const obs_state = {
    currentScene: null, // string
    audio_sources: {}, // { volume: 0, muted: false }
    is_recording: null, // bool
    is_streaming: null, // bool
    is_recording_paused: null, // bool
    is_muted: null, // bool
    recording_time: null, // string
    streaming_time: null, // string
};

function sendSingleStatus(key, value) {
    const message = {
        key,
        value,
        type: 'singleStatus'
    };
    wss.clients.forEach(client => {
        client.send(JSON.stringify(message));
    });
}

function sendEvent(data) {
    switch (data.updateType) {
        case 'SourceMuteStateChanged':
        {
            if (data.sourceName === process.env.MICROPHONE_NAME) {
                obs_state.is_muted = data.muted;
                sendSingleStatus('is_muted', obs_state.is_muted);
            }
            const audio_source = obs_state.audio_sources[data.sourceName];
            if (typeof audio_source !== 'object') {
                obs_state.audio_sources[data.sourceName] = {
                    volume: 0,
                    muted: data.muted
                };
            } else {
                audio_source.muted = data.muted;
            }
            break;
        }
        case 'SourceVolumeChanged':
        {
            const audio_source = obs_state.audio_sources[data.sourceName];
            if (typeof audio_source !== 'object') {
                obs_state.audio_sources[data.sourceName] = {
                    volume: Math.round(data.volume * 100),
                    muted: false
                };
            } else {
                audio_source.volume = Math.round(data.volume * 100);
            }
            break;
        }
        default:
            console.log(`Unknown event: ${data.updateType}`);
            return;
    }
}

// proxy all OBS events to all connected websockets
obs.connect({ address: obs_address, password: process.env.OBS_SERVER_PASSWORD })
    .then(() => {
        obs.on('*', sendEvent);
        obs.on('SourcesActivated', sendEvent);
        obs.on('SourcesDeactivated', sendEvent);
        obs.on('StreamStatus', sendEvent);
        obs.on('RecordingStarted', sendEvent);
        obs.on('RecordingStopped', sendEvent);
        obs.on('RecordingPaused', sendEvent);
        obs.on('RecordingResumed', sendEvent);
        obs.on('SourceAudioMixersChanged', sendEvent);
        obs.on('SourceMuteStateChanged', sendEvent);
        obs.on('SourceVolumeChanged', sendEvent);
        console.log('OBS connected');
    })
    .catch((err) => {
        console.error(err);
    });

wss.on('connection', (ws) => {
    ws.on('message', (message) => {
        const data = JSON.parse(message);
        switch (data.type) {
            case 'fullStatus':
                const data = JSON.parse(JSON.stringify(obs_state));
                data.type = 'fullStatus';
                ws.send(JSON.stringify(data));
                break;
        }
    });
});

wss.on('listening', () => {
    console.log(`Server started at port ${wss.options.port}`);
});

setInterval(() => {
    console.log(obs_state);
}, 3000);