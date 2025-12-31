from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, text_sensor
from esphome.const import CONF_ID, CONF_TRIGGER_ID

AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor"]
MULTI_CONF = True

CONF_TICK_INTERVAL = "tick_interval"
CONF_SYNC_INTERVAL = "sync_interval"
CONF_MAX_DURATION = "max_duration"
CONF_INITIAL_SET_SECONDS = "initial_set_seconds"
CONF_ENABLE_HA_SYNC = "enable_ha_sync"
CONF_HA_STATE_SENSOR = "ha_state_sensor"
CONF_HA_REMAINING_SENSOR = "ha_remaining_sensor"

CONF_STATE = "state"
CONF_REMAINING_SECONDS = "remaining_seconds"
CONF_SET_SECONDS = "set_seconds"
CONF_RUNNING = "running"
CONF_PAUSED = "paused"
CONF_OVERDUE = "overdue"

CONF_ON_STARTED = "on_started"
CONF_ON_PAUSED = "on_paused"
CONF_ON_RESUMED = "on_resumed"
CONF_ON_CANCELLED = "on_cancelled"
CONF_ON_FINISHED = "on_finished"
CONF_ON_TICK = "on_tick"

CONF_SECONDS = "seconds"

# Distinct namespace to avoid collisions with any future built-in timer helpers
# while keeping the integration name short.
timer_ns = cg.esphome_ns.namespace("timer_ext")
TimerComponent = timer_ns.class_("TimerComponent", cg.Component)

TimerStartedTrigger = timer_ns.class_("TimerStartedTrigger", automation.Trigger.template(cg.bool_))
TimerPausedTrigger = timer_ns.class_("TimerPausedTrigger", automation.Trigger.template(cg.bool_))
TimerResumedTrigger = timer_ns.class_("TimerResumedTrigger", automation.Trigger.template(cg.bool_))
TimerCancelledTrigger = timer_ns.class_("TimerCancelledTrigger", automation.Trigger.template(cg.bool_))
TimerFinishedTrigger = timer_ns.class_("TimerFinishedTrigger", automation.Trigger.template(cg.bool_))
TimerTickTrigger = timer_ns.class_("TimerTickTrigger", automation.Trigger.template(cg.int_))

TimerStartAction = timer_ns.class_("TimerStartAction", automation.Action)
TimerPauseAction = timer_ns.class_("TimerPauseAction", automation.Action)
TimerResumeAction = timer_ns.class_("TimerResumeAction", automation.Action)
TimerCancelAction = timer_ns.class_("TimerCancelAction", automation.Action)
TimerSetSecondsAction = timer_ns.class_("TimerSetSecondsAction", automation.Action)

SENSOR_SCHEMA = sensor.sensor_schema(
    unit_of_measurement="s",
    accuracy_decimals=0,
    icon="mdi:timer-outline",
)

BINARY_SENSOR_SCHEMA = binary_sensor.binary_sensor_schema(
    icon="mdi:timer-outline",
)

TEXT_SENSOR_SCHEMA = text_sensor.text_sensor_schema(
    icon="mdi:timer-outline",
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TimerComponent),
            cv.Optional(CONF_TICK_INTERVAL, default="1s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_SYNC_INTERVAL, default="5s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_MAX_DURATION, default="7200s"): cv.positive_time_period,
            cv.Optional(CONF_INITIAL_SET_SECONDS, default=0): cv.uint32_t,
            cv.Optional(CONF_ENABLE_HA_SYNC, default=True): cv.boolean,
            cv.Optional(CONF_HA_STATE_SENSOR): cv.use_id(text_sensor.TextSensor),
            cv.Optional(CONF_HA_REMAINING_SENSOR): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_STATE): TEXT_SENSOR_SCHEMA,
            cv.Optional(CONF_REMAINING_SECONDS): SENSOR_SCHEMA,
            cv.Optional(CONF_SET_SECONDS): SENSOR_SCHEMA,
            cv.Optional(CONF_RUNNING): BINARY_SENSOR_SCHEMA,
            cv.Optional(CONF_PAUSED): BINARY_SENSOR_SCHEMA,
            cv.Optional(CONF_OVERDUE): BINARY_SENSOR_SCHEMA,
            cv.Optional(CONF_ON_STARTED): automation.validate_automation({cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TimerStartedTrigger)}),
            cv.Optional(CONF_ON_PAUSED): automation.validate_automation({cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TimerPausedTrigger)}),
            cv.Optional(CONF_ON_RESUMED): automation.validate_automation({cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TimerResumedTrigger)}),
            cv.Optional(CONF_ON_CANCELLED): automation.validate_automation({cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TimerCancelledTrigger)}),
            cv.Optional(CONF_ON_FINISHED): automation.validate_automation({cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TimerFinishedTrigger)}),
            cv.Optional(CONF_ON_TICK): automation.validate_automation({cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TimerTickTrigger)}),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

TIMER_ACTION_SCHEMA = cv.Schema({cv.Required(CONF_ID): cv.use_id(TimerComponent)})
TIMER_START_ACTION_SCHEMA = TIMER_ACTION_SCHEMA.extend({cv.Optional(CONF_SECONDS): cv.templatable(cv.int_)})
TIMER_SET_SECONDS_ACTION_SCHEMA = TIMER_ACTION_SCHEMA.extend({cv.Required(CONF_SECONDS): cv.templatable(cv.int_)})


@automation.register_action("timer.start", TimerStartAction, TIMER_START_ACTION_SCHEMA)
async def timer_start_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    if (seconds := config.get(CONF_SECONDS)) is not None:
        template = await cg.templatable(seconds, args, cg.int_)
        cg.add(var.set_seconds(template))
    return var


@automation.register_action("timer.pause", TimerPauseAction, TIMER_ACTION_SCHEMA)
async def timer_pause_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action("timer.resume", TimerResumeAction, TIMER_ACTION_SCHEMA)
async def timer_resume_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action("timer.cancel", TimerCancelAction, TIMER_ACTION_SCHEMA)
async def timer_cancel_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action("timer.set_seconds", TimerSetSecondsAction, TIMER_SET_SECONDS_ACTION_SCHEMA)
async def timer_set_seconds_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    template = await cg.templatable(config[CONF_SECONDS], args, cg.int_)
    cg.add(var.set_seconds(template))
    return var


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_tick_interval_ms(config[CONF_TICK_INTERVAL].total_milliseconds))
    cg.add(var.set_sync_interval_ms(config[CONF_SYNC_INTERVAL].total_milliseconds))
    cg.add(var.set_max_duration_seconds(config[CONF_MAX_DURATION].total_seconds))
    cg.add(var.set_initial_set_seconds(config[CONF_INITIAL_SET_SECONDS]))
    cg.add(var.set_enable_ha_sync(config[CONF_ENABLE_HA_SYNC]))

    if (ha_state := config.get(CONF_HA_STATE_SENSOR)) is not None:
        ha_state_sensor = await cg.get_variable(ha_state)
        cg.add(var.set_ha_state_sensor(ha_state_sensor))

    if (ha_remaining := config.get(CONF_HA_REMAINING_SENSOR)) is not None:
        ha_remaining_sensor = await cg.get_variable(ha_remaining)
        cg.add(var.set_ha_remaining_sensor(ha_remaining_sensor))

    if (state := config.get(CONF_STATE)) is not None:
        state_sensor = await text_sensor.new_text_sensor(state)
        cg.add(var.set_state_text_sensor(state_sensor))

    if (remaining := config.get(CONF_REMAINING_SECONDS)) is not None:
        remaining_sensor = await sensor.new_sensor(remaining)
        cg.add(var.set_remaining_seconds_sensor(remaining_sensor))

    if (set_seconds := config.get(CONF_SET_SECONDS)) is not None:
        set_seconds_sensor = await sensor.new_sensor(set_seconds)
        cg.add(var.set_set_seconds_sensor(set_seconds_sensor))

    if (running := config.get(CONF_RUNNING)) is not None:
        running_sensor = await binary_sensor.new_binary_sensor(running)
        cg.add(var.set_running_binary_sensor(running_sensor))

    if (paused := config.get(CONF_PAUSED)) is not None:
        paused_sensor = await binary_sensor.new_binary_sensor(paused)
        cg.add(var.set_paused_binary_sensor(paused_sensor))

    if (overdue := config.get(CONF_OVERDUE)) is not None:
        overdue_sensor = await binary_sensor.new_binary_sensor(overdue)
        cg.add(var.set_overdue_binary_sensor(overdue_sensor))

    for conf in config.get(CONF_ON_STARTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "from_ha")], conf)

    for conf in config.get(CONF_ON_PAUSED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "from_ha")], conf)

    for conf in config.get(CONF_ON_RESUMED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "from_ha")], conf)

    for conf in config.get(CONF_ON_CANCELLED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "from_ha")], conf)

    for conf in config.get(CONF_ON_FINISHED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "from_ha")], conf)

    for conf in config.get(CONF_ON_TICK, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.int_, "x")], conf)
