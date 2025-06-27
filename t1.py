import cv2
import numpy as np
from deepface import DeepFace
import os
import time
import urllib.request
import serial

# === CONFIGURATION ===
EMOJI_FOLDER = "./emojis"
EMOTION_LIST = ["angry","happy","sad","neutral"]

# GUI Settings
WINDOW_WIDTH = 1200
WINDOW_HEIGHT = 600
FONT = cv2.FONT_HERSHEY_SIMPLEX
SHOW_FACE_MESH = True
flash_on = False

# Serial connection to ESP32 (adjust port!)
arduino = serial.Serial("COM4", 115200)

# === Load Emojis ===
EMOJI_IMAGES = {}
for emotion in EMOTION_LIST:
    path = os.path.join(EMOJI_FOLDER, f"{emotion}.png")
    if os.path.exists(path):
        emoji = cv2.imread(path, cv2.IMREAD_UNCHANGED)
        if emoji is not None:
            EMOJI_IMAGES[emotion] = emoji
        else:
            print(f"⚠️ Could not load emoji for {emotion}")
    else:
        print(f"⚠️ Emoji missing: {emotion}.png")

# === GUI Layout ===
def draw_layout(frame, emotion="...", score="...", emoji_img=None, show_face=True):
    gui = np.ones((WINDOW_HEIGHT, WINDOW_WIDTH, 3), dtype=np.uint8) * 255

    # Left Panel
    cv2.rectangle(gui, (0, 0), (300, 600), (0, 0, 0), 2)
    cv2.putText(gui, "REACTION", (90, 40), FONT, 1, (0, 0, 0), 3)

    if emoji_img is not None:
        try:
            emoji_resized = cv2.resize(emoji_img, (150, 150))
            if emoji_resized.shape[2] == 4:
                alpha_s = emoji_resized[:, :, 3] / 255.0
                for c in range(3):
                    gui[70:220, 75:225, c] = (
                        alpha_s * emoji_resized[:, :, c] +
                        (1 - alpha_s) * gui[70:220, 75:225, c]
                    )
            else:
                gui[70:220, 75:225] = emoji_resized
        except Exception as e:
            print("⚠️ Emoji render failed:", e)

    cv2.putText(gui, "Detected Emotion:", (50, 300), FONT, 0.8, (0, 0, 0), 2)
    cv2.putText(gui, emotion.upper(), (80, 350), FONT, 1.2, (0, 0, 0), 3)

    # Middle Panel
    cv2.rectangle(gui, (300, 0), (900, 600), (0, 0, 0), 2)
    if show_face and frame is not None:
        try:
            resized_face = cv2.resize(frame, (480, 480))
            gui[50:530, 360:840] = resized_face
        except:
            pass

    # Accuracy below face
    cv2.rectangle(gui, (450, 540), (750, 580), (0, 0, 0), 2)
    cv2.putText(gui, f"ACCURACY: {score}%", (460, 570), FONT, 0.9, (0, 0, 0), 2)

    # Right Panel
    cv2.rectangle(gui, (900, 0), (1200, 600), (0, 0, 0), 2)
    # FLASH Button
    cv2.rectangle(gui, (950, 400), (1150, 450), (80, 80, 200), -1)
    cv2.putText(gui, "FLASH", (1000, 435), FONT, 1, (255, 255, 255), 2)
    # QUIT Button
    cv2.rectangle(gui, (950, 500), (1150, 550), (50, 50, 50), -1)
    cv2.putText(gui, "QUIT", (1000, 535), FONT, 1, (255, 255, 255), 2)

    return gui

def is_button_clicked(x, y):
    if 950 <= x <= 1150 and 500 <= y <= 550:
        return "quit"
    elif 950 <= x <= 1150 and 400 <= y <= 450:
        return "flash"
    return None

def main():
    global flash_on
    url = "http://192.168.4.1:81/stream"
    stream = urllib.request.urlopen(url)
    bytes_data = b''
    last_update = 0
    current_emotion = "..."
    current_conf = "0"
    current_emoji = None

    def on_mouse(event, x, y, flags, param):
        global flash_on
        if event == cv2.EVENT_LBUTTONDOWN:
            action = is_button_clicked(x, y)
            if action == "quit":
                print("🟥 Quit button clicked")
                arduino.write(b"flash_off\n")
                cv2.destroyAllWindows()
                exit()
            elif action == "flash":
                flash_on = not flash_on
                cmd = "flash_on\n" if flash_on else "flash_off\n"
                arduino.write(cmd.encode())
                print(f"⚡ Flash {'ON' if flash_on else 'OFF'}")

    cv2.namedWindow("Emotion Recognition GUI")
    cv2.setMouseCallback("Emotion Recognition GUI", on_mouse)

    while True:
        bytes_data += stream.read(1024)
        a = bytes_data.find(b'\xff\xd8')
        b = bytes_data.find(b'\xff\xd9')

        if a != -1 and b != -1:
            jpg = bytes_data[a:b+2]
            bytes_data = bytes_data[b+2:]
            frame = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)

            if frame is None:
                continue

            if time.time() - last_update >= 1:
                try:
                    result = DeepFace.analyze(frame, actions=['emotion'], enforce_detection=False)
                    current_emotion = result[0]['dominant_emotion']
                    current_conf = str(round(result[0]['emotion'][current_emotion], 2))
                    current_emoji = EMOJI_IMAGES.get(current_emotion.lower(), None)
                    arduino.write((current_emotion + "\n").encode())
                    last_update = time.time()
                except Exception as e:
                    print("⚠️ Detection error:", e)

            gui_frame = draw_layout(frame, current_emotion, current_conf, current_emoji, show_face=SHOW_FACE_MESH)
            cv2.imshow("Emotion Recognition GUI", gui_frame)

            if cv2.waitKey(1) & 0xFF == 27:
                break

    arduino.write(b"flash_off\n")
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
