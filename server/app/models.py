from enum import Enum
from time import time

from pydantic import BaseModel, Field


class Direction(str, Enum):
    STOPPED = "stopped"
    FORWARD = "forward"
    REVERSE = "reverse"
    UNKNOWN = "unknown"


class PowerState(str, Enum):
    STANDBY = "standby"
    STARTING = "starting"
    RUNNING = "running"
    FAULT = "fault"
    OFF = "off"


class LocomotiveTelemetry(BaseModel):
    locomotive_id: str
    timestamp: float = Field(default_factory=time)
    power_state: PowerState = PowerState.STANDBY
    dynamo_enabled: bool = False
    main_power_good: bool = False
    marker_id: int | None = None
    direction: Direction = Direction.UNKNOWN
    target_speed_cm_s: float = 0.0
    estimated_speed_cm_s: float = 0.0
    pwm_duty: float = Field(default=0.0, ge=0.0, le=1.0)
    motor_temperature_c: float | None = None
    mosfet_temperature_c: float | None = None
    front_lights: bool = False
    rear_lights: bool = False
    fault: str | None = None


class TurnoutState(BaseModel):
    turnout_id: str
    position: str
    locked: bool = False


class LightRule(BaseModel):
    rule_id: str
    target_id: str
    enabled: bool = True
    on_hour: int = Field(ge=0, le=23)
    off_hour: int = Field(ge=0, le=23)


class Command(BaseModel):
    target_id: str
    command: str
    value: str | int | float | bool | None = None

