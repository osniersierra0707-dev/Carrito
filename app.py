from flask import Flask, render_template, jsonify
import requests

app = Flask(__name__)
ESP32_URL = "http://192.168.0.6"   # pon aquí la IP que imprime la ESP32

MODOS = ["manual", "automatico", "seguidor"]
ACCIONES = ["adelante", "atras", "izquierda", "derecha", "detener"]


@app.route("/")
def inicio():
    return render_template("index.html")


@app.route("/estado")
def estado():
    ip = ESP32_URL.replace("http://", "")
    try:
        respuesta = requests.get(f"{ESP32_URL}/", timeout=2)
        if respuesta.status_code == 200:
            return jsonify({"conectado": True, "ip": ip})
    except requests.RequestException:
        pass
    return jsonify({"conectado": False, "ip": ip})


@app.route("/modo/<tipo>", methods=["POST"])
def cambiar_modo(tipo):
    if tipo not in MODOS:
        return jsonify({"ok": False}), 400
    try:
        requests.get(f"{ESP32_URL}/modo/{tipo}", timeout=2)
        return jsonify({"ok": True, "modo": tipo})
    except requests.RequestException:
        return jsonify({"ok": False, "error": "Error de comunicación"}), 502


@app.route("/mover/<direccion>", methods=["POST"])
def mover(direccion):
    if direccion not in ACCIONES:
        return jsonify({"ok": False}), 400
    try:
        requests.get(f"{ESP32_URL}/{direccion}", timeout=2)
        return jsonify({"ok": True})
    except requests.RequestException:
        return jsonify({"ok": False}), 502


@app.route("/velocidad/<int:valor>", methods=["POST"])
def velocidad(valor):
    if valor < 0 or valor > 100:
        return jsonify({"ok": False, "error": "Valor fuera de rango (0-100)"}), 400
    try:
        requests.get(f"{ESP32_URL}/velocidad", params={"valor": valor}, timeout=2)
        return jsonify({"ok": True, "velocidad": valor})
    except requests.RequestException:
        return jsonify({"ok": False, "error": "Error de comunicación"}), 502


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)