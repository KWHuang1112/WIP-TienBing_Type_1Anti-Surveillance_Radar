from flask import Flask
import pyautogui
import time

app = Flask(__name__)

@app.route('/trigger')
# def trigger():
    # print("偵測到接近物！正在切換螢幕...")
    
    # # 選項 A: 顯示桌面 (Win + D)
    # pyautogui.hotkey('win', 'd')
    
    # # 選項 B: 切換視窗 (Alt + Tab)
    # # pyautogui.hotkey('alt', 'tab')
    
    # # 選項 C: 鎖定電腦 (Win + L)
    # # pyautogui.hotkey('win', 'l')

    # return "OK", 200

def trigger():
    print(">>> 收到觸發指令！執行切換...")
    try:
        # 確保 pyautogui 有一點點緩衝時間
        pyautogui.PAUSE = 0.1
        # 強制顯示桌面
        pyautogui.keyDown('win')
        pyautogui.press('d')
        pyautogui.keyUp('win')
        return "SUCCESS", 200
    except Exception as e:
        print(f"錯誤：{e}")
        return "FAIL", 500


if __name__ == '__main__':
    # 注意：電腦必須連上 ESP32 的 WiFi (Radar_System)
    # 並且確認電腦在該網路下的 IP 是 192.168.4.2 (通常是第一個連入者)
    app.run(host='192.168.4.2', port=5000) #0.0.0.0