
const mSec = 1000;

export const enum ScheduleState {
    Default = "DEFAULT",
    AddressCopied = "ADDRESS_COPIED",
    BalanceChangePresumed = "BALANCE_CHANGE_PRESUMED"
}

class ScheduleItem {
    constructor(
        readonly interval: number,
        readonly duration: number = 0,
        readonly latency: number = 0,
    ) {
        this.latency = latency;
        this.interval = interval;
        this.duration = duration;
    }
}

type Schedule = Record <ScheduleState, ScheduleItem>;

export class Scheduler {
    private readonly _schedule: Schedule;
    private _isActive: boolean;
    private readonly _action: () => void;

    private _latencyTimeout: ReturnType<typeof setTimeout> | null = null;
    private _actionInterval: ReturnType<typeof setInterval> | null = null;
    private _durationTimeout: ReturnType<typeof setTimeout> | null = null;

    constructor(readonly schedule: Schedule, action: () => void) {
        this._schedule = schedule;
        this._isActive = false;
        this._action = action;
    }

    activate() {
        this._isActive = true;
    }

    deactivate() {
        this._isActive = false;
    }

    _runAction() {
        if (this._isActive && this._action != null) {
            this._action();
        }
    }

    _runActionInterval(interval: number) {
        // this._runAction();
        if (0 < interval) {
            this._actionInterval = setInterval(() => { this._runAction(); }, interval * mSec);
        }
    }

    _startScheduleItem(item: ScheduleItem) {
        if (0 < item.latency) {
            this._latencyTimeout = setTimeout(() => {
                this._runActionInterval(item.interval);
            }, item.latency * mSec);
        } else {
            this._runActionInterval(item.interval);
        }

        if (0 < item.duration) {
            this._durationTimeout = setTimeout(() => {
                this.changeStateTo(ScheduleState.Default);
            }, item.duration * mSec);
        }
    }

    _stopRunningScheduleItem() {
        if (this._latencyTimeout != null) {
            clearTimeout(this._latencyTimeout);
            this._latencyTimeout = null;
        }
        if (this._actionInterval != null) {
            clearInterval(this._actionInterval);
            this._actionInterval = null;
        }
        if (this._durationTimeout != null) {
            clearTimeout(this._durationTimeout);
            this._durationTimeout = null;
        }
    }

    changeStateTo(state: ScheduleState) {
        this._stopRunningScheduleItem();
        this._startScheduleItem(this._schedule[state])
    }
}

export const defaultSchedule: Schedule  = {
    [ScheduleState.Default] : new ScheduleItem(
        600,  // interval = 10 minutes
    ),
    [ScheduleState.AddressCopied]: new ScheduleItem(
        30,  // interval = 30 seconds
        1200,  // duration = 20 minutes
        120,  // first run latency = 2 minutes
    ),
    [ScheduleState.BalanceChangePresumed]: new ScheduleItem(
        30,  // interval = 30 seconds
        1200,  // duration = 20 minutes
    )
};
