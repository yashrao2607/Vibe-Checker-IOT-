import argparse
import csv
import threading
import time
from pathlib import Path

import librosa
import numpy as np
import pygame

try:
    import serial
except Exception:
    serial = None


def score_to_mood(score: int) -> str:
    if score < 25:
        return "CALM"
    if score < 50:
        return "ACTIVE"
    if score < 75:
        return "EXCITED"
    return "CHAOTIC"


def analyze_audio_frames(audio_path: str, interval: float, duration: float = 0.0):
    load_duration = None if duration <= 0 else duration
    y, sr = librosa.load(audio_path, sr=22050, mono=True, duration=load_duration)
    hop = max(1, int(sr * interval))
    win = max(hop * 2, 2048)

    rms = librosa.feature.rms(y=y, frame_length=win, hop_length=hop)[0]
    zcr = librosa.feature.zero_crossing_rate(y, frame_length=win, hop_length=hop)[0]
    centroid = librosa.feature.spectral_centroid(y=y, sr=sr, n_fft=win, hop_length=hop)[0]

    rms_n = rms / max(np.max(rms), 1e-9)
    zcr_n = zcr / max(np.max(zcr), 1e-9)
    cen_n = centroid / max(np.max(centroid), 1e-9)

    raw = (0.65 * rms_n) + (0.20 * zcr_n) + (0.15 * cen_n)
    smooth = np.copy(raw)
    for i in range(1, len(smooth)):
        smooth[i] = (0.85 * smooth[i - 1]) + (0.15 * smooth[i])

    scores = np.clip((smooth * 100.0).astype(int), 0, 100)

    for i, score in enumerate(scores):
        t = i * interval
        mood = score_to_mood(int(score))
        yield t, mood, int(score)


def play_audio(audio_path: str):
    pygame.mixer.init()
    pygame.mixer.music.load(audio_path)
    pygame.mixer.music.play()


def main():
    parser = argparse.ArgumentParser(
        description="Play an audio file and stream crowd mood to Arduino in real time."
    )
    parser.add_argument("--audio", required=True, help="Path to audio file (mp3/wav)")
    parser.add_argument("--port", default="", help="Arduino COM port, e.g. COM5 (optional)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    parser.add_argument("--interval", type=float, default=0.4, help="Mood update interval (seconds)")
    parser.add_argument(
        "--duration",
        type=float,
        default=0.0,
        help="Analyze only first N seconds (0 = full file)",
    )
    parser.add_argument(
        "--no-play",
        action="store_true",
        help="Analyze without playing audio",
    )
    parser.add_argument(
        "--out",
        default="audio_mood_results.csv",
        help="CSV output path for timeline results",
    )
    args = parser.parse_args()

    audio_path = str(Path(args.audio).expanduser())
    if not Path(audio_path).exists():
        raise FileNotFoundError(f"Audio file not found: {audio_path}")

    ser = None
    if args.port:
        if serial is None:
            raise RuntimeError("pyserial is not installed. Install dependencies first.")
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2.0)  # wait for Arduino reset

    try:
        if not args.no_play:
            play_thread = threading.Thread(target=play_audio, args=(audio_path,), daemon=True)
            play_thread.start()

        start = time.time()
        duration = max(0.0, args.duration)
        mood_counts = {"CALM": 0, "ACTIVE": 0, "EXCITED": 0, "CHAOTIC": 0}
        rows = []

        for t_sec, mood, score in analyze_audio_frames(audio_path, args.interval, duration=duration):

            now = time.time() - start
            wait = t_sec - now
            if wait > 0:
                time.sleep(wait)

            line = f"MOOD:{mood},SCORE:{score}\n"
            if ser is not None:
                ser.write(line.encode("utf-8"))
            print(line.strip(), flush=True)
            mood_counts[mood] += 1
            rows.append((round(t_sec, 2), mood, score))

        with open(args.out, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["time_sec", "mood", "score"])
            writer.writerows(rows)

        total = sum(mood_counts.values()) or 1
        print("\n=== Summary ===")
        for m in ["CALM", "ACTIVE", "EXCITED", "CHAOTIC"]:
            pct = (mood_counts[m] * 100.0) / total
            print(f"{m}: {mood_counts[m]} samples ({pct:.1f}%)")
        dominant = max(mood_counts, key=mood_counts.get)
        print(f"Dominant mood: {dominant}")
        print(f"CSV saved: {args.out}")

        while not args.no_play and pygame.mixer.music.get_busy():
            time.sleep(0.1)
    finally:
        try:
            if not args.no_play:
                pygame.mixer.music.stop()
                pygame.mixer.quit()
        except Exception:
            pass
        if ser is not None:
            ser.close()


if __name__ == "__main__":
    main()
