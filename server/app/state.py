from collections import deque

from .models import LightRule, LocomotiveTelemetry, TurnoutState


class DioramaState:
    def __init__(self) -> None:
        self.locomotives: dict[str, LocomotiveTelemetry] = {}
        self.turnouts: dict[str, TurnoutState] = {}
        self.light_rules: dict[str, LightRule] = {}
        self.events: deque[str] = deque(maxlen=200)

    def update_locomotive(self, telemetry: LocomotiveTelemetry) -> None:
        self.locomotives[telemetry.locomotive_id] = telemetry
        marker = telemetry.marker_id if telemetry.marker_id is not None else "-"
        self.events.append(
            f"{telemetry.locomotive_id}: {telemetry.power_state} "
            f"marker={marker} speed={telemetry.estimated_speed_cm_s:.1f}cm/s"
        )

    def snapshot(self) -> dict:
        return {
            "locomotives": {
                locomotive_id: telemetry.model_dump()
                for locomotive_id, telemetry in self.locomotives.items()
            },
            "turnouts": {
                turnout_id: turnout.model_dump()
                for turnout_id, turnout in self.turnouts.items()
            },
            "light_rules": {
                rule_id: rule.model_dump()
                for rule_id, rule in self.light_rules.items()
            },
            "events": list(self.events),
        }


state = DioramaState()

