import { useRef, useState } from 'react';
import { useModalEscape } from '../hooks/useModalEscape';

type WebcamModalProps = {
    open: boolean;
    targetLabel: string;
    busy: boolean;
    onClose: () => void;
};

function WebcamModal({
    open,
    targetLabel,
    busy,
    onClose,
}: WebcamModalProps) {
    useModalEscape(open, onClose);

    const videoRef = useRef<HTMLVideoElement | null>(null);
    const streamRef = useRef<MediaStream | null>(null);
    const mediaRecorderRef = useRef<MediaRecorder | null>(null);
    const chunksRef = useRef<Blob[]>([]);

    const [recording, setRecording] = useState(false);
    const [hasCamera, setHasCamera] = useState(false);
    const [status, setStatus] = useState('Idle');

    if (!open) return null;

    const stopAllTracks = () => {
        if (mediaRecorderRef.current && mediaRecorderRef.current.state !== 'inactive') {
            mediaRecorderRef.current.stop();
        }

        if (streamRef.current) {
            streamRef.current.getTracks().forEach((track) => {
                track.stop();
            });
            streamRef.current = null;
        }

        if (videoRef.current) {
            videoRef.current.pause();
            videoRef.current.srcObject = null;
        }

        setRecording(false);
        setHasCamera(false);
    };

    const handleClose = () => {
        stopAllTracks();
        setStatus('Idle');
        onClose();
    };

    const handleStartWebcam = async () => {
        try {
            if (streamRef.current) {
                setStatus('Camera already started');
                return;
            }

            setStatus('Requesting camera permission...');

            const mediaStream = await navigator.mediaDevices.getUserMedia({
                video: true,
                audio: false,
            });

            streamRef.current = mediaStream;

            if (videoRef.current) {
                videoRef.current.srcObject = mediaStream;
                await videoRef.current.play();
            }

            setHasCamera(true);
            setStatus('Camera started');
        } catch (error) {
            console.error('Cannot access webcam:', error);
            setStatus('Cannot open webcam');
            alert('Không mở được webcam. Hãy kiểm tra quyền camera trong trình duyệt.');
        }
    };

    const handleStartRecord = () => {
        const stream = streamRef.current;

        if (!stream) {
            alert('Hãy mở webcam trước.');
            return;
        }

        try {
            chunksRef.current = [];

            const recorder = new MediaRecorder(stream);

            recorder.ondataavailable = (event: BlobEvent) => {
                if (event.data.size > 0) {
                    chunksRef.current.push(event.data);
                }
            };

            recorder.onstop = () => {
                const blob = new Blob(chunksRef.current, { type: 'video/webm' });
                const url = URL.createObjectURL(blob);

                const timestamp = new Date().toISOString().replace(/[:.]/g, '-');

                const link = document.createElement('a');
                link.href = url;
                link.download = `webcam-record-${timestamp}.webm`;
                document.body.appendChild(link);
                link.click();
                document.body.removeChild(link);

                URL.revokeObjectURL(url);
                setStatus('Recording saved');
            };

            mediaRecorderRef.current = recorder;
            recorder.start();

            setRecording(true);
            setStatus('Recording...');
        } catch (error) {
            console.error('Cannot start recording:', error);
            setStatus('Cannot start recording');
            alert('Không bắt đầu ghi hình được.');
        }
    };

    const handleStopRecord = () => {
        if (!mediaRecorderRef.current || mediaRecorderRef.current.state === 'inactive') {
            return;
        }

        mediaRecorderRef.current.stop();
        setRecording(false);
        setStatus('Stopping recording...');
    };

    const handleStopWebcam = () => {
        stopAllTracks();
        setStatus('Camera stopped');
    };

    return (
        <div className="modal-backdrop" onClick={handleClose} role="presentation">
            <div
                className="modal-panel process-modal"
                onClick={(e) => e.stopPropagation()}
                role="dialog"
                aria-modal="true"
                aria-labelledby="webcam-title"
            >
                <div className="modal-header">
                    <div>
                        <h2 id="webcam-title">Webcam Local</h2>
                        <p className="modal-sub">{targetLabel}</p>
                        <p className="modal-sub">Status: {status}</p>
                    </div>

                    <button className="btn modal-close" onClick={handleClose} type="button">
                        Close
                    </button>
                </div>

                <div className="process-modal-toolbar">
                    <button
                        className="btn primary"
                        disabled={busy || hasCamera}
                        onClick={handleStartWebcam}
                        type="button"
                    >
                        Start webcam
                    </button>

                    <button
                        className="btn"
                        disabled={busy || !hasCamera || recording}
                        onClick={handleStartRecord}
                        type="button"
                    >
                        Start record
                    </button>

                    <button
                        className="btn btn-danger"
                        disabled={busy || !recording}
                        onClick={handleStopRecord}
                        type="button"
                    >
                        Stop record
                    </button>

                    <button
                        className="btn"
                        disabled={busy || !hasCamera}
                        onClick={handleStopWebcam}
                        type="button"
                    >
                        Stop webcam
                    </button>
                </div>

                <div style={{ marginTop: 16 }}>
                    <video
                        ref={videoRef}
                        autoPlay
                        playsInline
                        muted
                        style={{
                            width: '100%',
                            maxHeight: 420,
                            borderRadius: 10,
                            background: '#000',
                            objectFit: 'cover',
                        }}
                    />
                </div>
            </div>
        </div>
    );
}

export default WebcamModal;