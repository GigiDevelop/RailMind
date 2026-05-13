import asyncio
import json
from pathlib import Path

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles

from .models import Command, LightRule, LocomotiveTelemetry, TurnoutState
from .state import state

app = FastAPI(title="RailMind Server")
STATIC_DIR = Path(__file__).parent / "static"
app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")


@app.get("/")
def dashboard() -> FileResponse:
    return FileResponse(STATIC_DIR / "index.html")


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/api/state")
def get_state() -> dict:
    return state.snapshot()


@app.post("/api/telemetry/locomotive")
def post_locomotive_telemetry(telemetry: LocomotiveTelemetry) -> dict[str, str]:
    state.update_locomotive(telemetry)
    return {"status": "accepted"}


@app.post("/api/turnouts/{turnout_id}")
def set_turnout(turnout_id: str, turnout: TurnoutState) -> dict[str, str]:
    state.turnouts[turnout_id] = turnout
    state.events.append(f"turnout {turnout_id}: {turnout.position}")
    return {"status": "accepted"}


@app.post("/api/lights/rules/{rule_id}")
def set_light_rule(rule_id: str, rule: LightRule) -> dict[str, str]:
    state.light_rules[rule_id] = rule
    state.events.append(f"light rule {rule_id}: {rule.target_id}")
    return {"status": "accepted"}


@app.post("/api/commands")
def enqueue_command(command: Command) -> dict[str, str]:
    # La coda reale verso gateway ESP32-S2 verra implementata nel passo successivo.
    state.events.append(f"command {command.target_id}: {command.command}={command.value}")
    return {"status": "queued"}


@app.websocket("/ws/telemetry")
async def telemetry_websocket(websocket: WebSocket) -> None:
    await websocket.accept()
    try:
        while True:
            await websocket.send_text(json.dumps(state.snapshot()))
            await asyncio.sleep(1.0)
    except WebSocketDisconnect:
        return

