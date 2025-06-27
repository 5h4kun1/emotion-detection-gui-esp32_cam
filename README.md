# Real-Time Emotion Detection System using ESP32-CAM & Python GUI

This project detects facial emotions using an ESP32-CAM and a Python GUI with DeepFace. The detected emotion is sent over serial to an ESP32 which shows an emoji on an LCD and lights up corresponding LEDs.

---

## 🔥 Features

- Live video stream from ESP32-CAM
- Emotion detection using DeepFace
- Emojis and labels shown in GUI
- Sends emotion via Serial to ESP32
- Flash ON/OFF button in GUI
- LED and emoji display via ESP32 Arduino code

---

## 🧠 Emotions Supported

| Emotion  | Emoji | LED Color |
|----------|--------|-----------|
| Happy    | 😀     | Green     |
| Sad      | 😢     | Blue      |
| Angry    | 😠     | Red       |
| Neutral  | 😐     | Blue      |

---

## 📁 Project Structure

<pre>
emotion-detection-gui-esp32/
├── Arduino/
│   └── f3.ino                  # ESP32 code for serial emoji + LED
│
├── Python/
│   ├── t1.py                   # Main GUI (DeepFace + OpenCV)
│   └── emojis/
│       ├── happy.png
│       ├── sad.png
│       ├── angry.png
│       └── neutral.png         # Transparent PNGs for overlay
│
├── requirements.txt            # Python dependencies
├── README.md                   # This file
└── .gitignore                  # Ignore cache, logs, temp
</pre>

---

## 🖥️ How to Run the Python GUI

1. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

2. In `t1.py`, update:
   - Serial port (e.g., `COM4`)
   - ESP32-CAM stream URL (`http://192.168.4.1:81/stream`)

3. Run it:
   ```bash
   cd Python
   python t1.py
   ```

---

## 🔌 ESP32 Arduino Setup

1. Open `f3.ino` in Arduino IDE
2. Select board: `AI Thinker ESP32-CAM` or your board
3. Upload code (via FTDI or USB)
4. Connect:
   - I2C LCD for emoji text
   - RGB LEDs for emotion indicator

The ESP32 listens on Serial and reacts based on received emotion strings (`happy`, `sad`, etc.).

---

## 📦 requirements.txt

```txt
opencv-python
numpy
deepface
pyserial
```

---

