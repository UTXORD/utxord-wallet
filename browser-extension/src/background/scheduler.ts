
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


export class Watchdog {
    public static readonly names = {
        POPUP_WATCHDOG: "POPUP_WATCHDOG"
    }
    private static readonly DEFAULT_TIMEOUT: number = 10;
    private static readonly DEFAULT_CHECK_INTERVAL: number = 10 * mSec;
    private _timeout: number = Watchdog.DEFAULT_TIMEOUT;
    private _action: (() => void) | null = null;
    
    private static _instances: Record<string, Watchdog> = {};
    private readonly _name: string;
    private _checkInterval: ReturnType<typeof setInterval> | null = null;

    private constructor(name: string) {
        this._name = name;
        this._action = null;
        this._timeout = Watchdog.DEFAULT_TIMEOUT;
        this._checkInterval = null;
    }

    public get name() {
        return this._name;
    }

    public static getNamedInstance(name: string): Watchdog {
        if (!Watchdog._instances[name]) {
            Watchdog._log("create instance");
            Watchdog._instances[name] = new Watchdog(name);
        }
        Watchdog._log("return instance");
        return Watchdog._instances[name];
    }

    public set actionOnTimeout(action: (() => void) | null) {
        Watchdog._log("set on-timeout action");
        this._action = action;
    }

    public set timeout(timeout: number) {
        Watchdog._log("set timeout");
        this._timeout = timeout;
    }

    private doAction() {
        Watchdog._log("do timeout action in case it has been set");
        if (this._action != null) {
            Watchdog._log("do action");
            this._action();
        }
    }

    public reset() {
        Watchdog._log("reset timeout");
        this._timeout = Watchdog.DEFAULT_TIMEOUT;
    }

    private _check() {
        this._timeout = Math.max(0, this._timeout-1);
        Watchdog._log(`check timeout: ${this._timeout}`);
        if (0 == this._timeout) {
            this.doAction();
        }
    }

    public run() {
        Watchdog._log("run");
        this._checkInterval = setInterval(() => {
          this._check();
        }, Watchdog.DEFAULT_CHECK_INTERVAL);
    }

    public stop() {
        Watchdog._log("stop");
        if (this._checkInterval) {
            clearInterval(this._checkInterval);
            this._checkInterval = null;
        }
    }

    private static _log(msg: string) {
        console.debug(`===== Watchdog: ${msg}`);
    }
}

export class Scheduler {
    private static _instance: Scheduler;

    private _schedule: Schedule | null = null;
    private _isActive: boolean = false;
    private _action: (() => void) | null = null;

    private _latencyTimeout: ReturnType<typeof setTimeout> | null = null;
    private _actionInterval: ReturnType<typeof setInterval> | null = null;
    private _durationTimeout: ReturnType<typeof setTimeout> | null = null;

    private constructor() {
        this._isActive = false;
    }

    public static getInstance(): Scheduler {
        if (!Scheduler._instance) {
            Scheduler._log("create instance");
            Scheduler._instance = new Scheduler();
        }
        Scheduler._log("return instance");
        return Scheduler._instance;
    }

    public set schedule(schedule: Schedule) {
        Scheduler._log("set schedule");
        this._schedule = schedule;
    }
    
    public set action(action: () => void) {
        Scheduler._log("set action");
        this._action = action;
    }
    
    activate() {
        Scheduler._log("activate");
        this._isActive = true;
    }

    deactivate() {
        Scheduler._log("deactivate");
        this._isActive = false;
    }

    private doAction() {
        Scheduler._log("do scheduled action in case it has been set");
        if (this._isActive && this._action != null) {
            Scheduler._log("do action");
            this._action();
        }
    }

    private runActionInterval(interval: number) {
        Scheduler._log("run action interval");
        this.doAction();
        if (0 < interval) {
            this._actionInterval = setInterval(() => { this.doAction(); }, interval * mSec);
        }
    }

    private startScheduleItem(item: ScheduleItem) {
        Scheduler._log(`startScheduleItem: ${item}`);
        if (0 < item.latency) {
            this._latencyTimeout = setTimeout(() => {
                this.runActionInterval(item.interval);
            }, item.latency * mSec);
        } else {
            this.runActionInterval(item.interval);
        }
        if (0 < item.duration) {
            this._durationTimeout = setTimeout(() => {
                this.changeStateTo(ScheduleState.Default);
            }, (item.latency + item.duration) * mSec);
        }
    }

    private stopRunningScheduleItem() {
        Scheduler._log("stopRunningScheduleItem");
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

    public changeStateTo(state: ScheduleState) {
        Scheduler._log(`changeStateTo: ${state}`);
        this.stopRunningScheduleItem();
        if (this._schedule && this._schedule[state]) {
            this.startScheduleItem(this._schedule[state])
        }
    }

    private static _log(msg: string) {
        console.debug(`===== Scheduler: ${msg}`);
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

export const debugSchedule: Schedule  = {
    [ScheduleState.Default] : new ScheduleItem(
        60,  // interval = 10 minutes
    ),
    [ScheduleState.AddressCopied]: new ScheduleItem(
        3,  // interval = 30 seconds
        120,  // duration = 20 minutes
        12,  // first run latency = 2 minutes
    ),
    [ScheduleState.BalanceChangePresumed]: new ScheduleItem(
        3,  // interval = 30 seconds
        120,  // duration = 20 minutes
    )
};
