export const enum ScheduleName {
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

type Schedule = Record <ScheduleName, ScheduleItem>;


export class Watchdog {
    private static readonly NAME_PREFIX: string = "utxord_wallet.watchdog";
    public static readonly names = {
        POPUP_WATCHDOG: "POPUP_WATCHDOG"
    }

    private static readonly DEFAULT_TIMEOUT: number = 3;
    private static readonly DEFAULT_CHECK_INTERVAL: number = 1;
    private _timeoutValue : number = Watchdog.DEFAULT_TIMEOUT;
    private _timeout: number;
    private _action: (() => void) | null = null;

    private static _instances: Record<string, Watchdog> = {};
    private readonly _name: string;
    private readonly _nameOfCheckAlarm: string;

    private constructor(name: string) {
        this._name = `${Watchdog.NAME_PREFIX}.${name}`;
        this._nameOfCheckAlarm = `${this._name}.check_alarm`;
        this._action = null;
        this._timeout = this._timeoutValue;
        chrome.alarms.onAlarm.addListener(async (alarm) => {
            if (alarm.name == this._nameOfCheckAlarm) {
                this._check();
            }
        });
    }

    public get name() {
        return this._name;
    }

    public static getNamedInstance(name: string): Watchdog {
        if (!Watchdog._instances[name]) {
            Watchdog._log("create instance");
            Watchdog._instances[name] = new Watchdog(name);
        }
        // Watchdog._log("return instance");
        return Watchdog._instances[name];
    }

    public set onTimeoutAction(action: (() => void) | null) {
        Watchdog._log("set on-timeout action");
        this._action = action;
    }

    public set timeout(timeout: number) {
        Watchdog._log("set timeout");
        this._timeoutValue = timeout;
    }

    private doAction() {
        // Watchdog._log("do timeout action in case it has been set");
        if (this._action != null) {
            Watchdog._log("do timeout action");
            this._action();
        }
    }

    public reset() {
        Watchdog._log("reset timeout");
        this._timeout = this._timeoutValue;
    }

    private _check() {
        if (0 == this._timeout) {
            return;
        }
        Watchdog._log(`check timeout: ${this._timeout}`);
        this._timeout = Math.max(0, this._timeout-1);
        if (0 == this._timeout) {
            this.doAction();
        }
    }

    public async run() {
        Watchdog._log("run");
        await chrome.alarms.create(
            this._nameOfCheckAlarm,
            { periodInMinutes: Watchdog.DEFAULT_CHECK_INTERVAL },
        );
    }

    public async stop() {
        Watchdog._log("stop");
        await chrome.alarms.clear(this._nameOfCheckAlarm);
    }

    private static _log(msg: string) {
        console.debug(`===== Watchdog: ${msg}`);
    }
}


export class Scheduler {
    private static _instance: Scheduler;
    private static readonly NAME_PREFIX: string = "utxord_wallet.scheduler";
    private static readonly DEFAULT_NAME: string = "periodic_query_scheduler";
    private readonly _name: string;
    private readonly _nameOfLatencyAlarm: string;
    private readonly _nameOfIntervalAlarm: string;
    private readonly _nameOfDurationAlarm: string;
    private _schedule: Schedule | null = null;
    private _currentScheduleItem: ScheduleItem | null = null;
    private _isActive: boolean = false;
    private _action: (() => void) | null = null;

    private constructor(name: string) {
        this._name = `${Scheduler.NAME_PREFIX}.${name}`;
        this._nameOfLatencyAlarm = `${this._name}.latency_alarm`;
        this._nameOfIntervalAlarm = `${this._name}.interval_alarm`;
        this._nameOfDurationAlarm = `${this._name}.duration_alarm`;
        this._isActive = false;
        chrome.alarms.onAlarm.addListener(async (alarm) => {
            switch (alarm.name) {
                case this._nameOfLatencyAlarm: {
                    await this.runActionInterval(this._currentScheduleItem?.interval || 0);
                    break;
                }
                case this._nameOfIntervalAlarm: {
                    this.doAction();
                    break;
                }
                case this._nameOfDurationAlarm: {
                    await this.changeScheduleTo(ScheduleName.Default);
                    break;
                }
            }
        });

    }

    public static getInstance(): Scheduler {
        if (!Scheduler._instance) {
            Scheduler._log("create instance");
            Scheduler._instance = new Scheduler(Scheduler.DEFAULT_NAME);
        }
        // Scheduler._log("return instance");
        return Scheduler._instance;
    }

    public get name() {
        return this._name;
    }

    public set schedule(schedule: Schedule) {
        Scheduler._log("set schedule");
        this._schedule = schedule;
    }

    public set action(action: (() => void) | null) {
        Scheduler._log("set action");
        this._action = action;
    }

    activate() {
        if (this._isActive) {
            return;
        }
        Scheduler._log("activate");
        this._isActive = true;
    }

    deactivate() {
        if (!this._isActive) {
            return;
        }
        Scheduler._log("deactivate");
        this._isActive = false;
    }

    private doAction() {
        Scheduler._log(`--- ${new Date().toLocaleString()} do scheduled action in case it has been set. ` +
            `isActive: ${this._isActive}, action: ${this._action}`);
        if (this._isActive && this._action != null) {
            Scheduler._log("do action");
            this._action();
        }
    }

    private async runActionInterval(interval: number) {
        Scheduler._log("run action interval");
        this.doAction();
        if (0 < interval) {
            await chrome.alarms.create(
                this._nameOfIntervalAlarm,
                { periodInMinutes: interval / 60.0 }
            );
        }
    }

    private async startScheduleItem(item: ScheduleItem) {
        Scheduler._log(`--- ${new Date().toLocaleString()} startScheduleItem, latency: ${item.latency}, interval: ${item.interval}, duration: ${item.duration}`);
        this._currentScheduleItem = item;
        if (0 < item.latency) {
            await chrome.alarms.create(
                this._nameOfLatencyAlarm,
                { delayInMinutes: item.latency / 60.0 }
            );
        } else {
            await this.runActionInterval(item.interval);
        }
        if (0 < item.duration) {
            await chrome.alarms.create(
                this._nameOfDurationAlarm,
                {delayInMinutes: (item.latency + item.duration) / 60.0}
            );
        }
    }

    private async stopRunningScheduleItem() {
        Scheduler._log("--- ${new Date().toLocaleString()} stopRunningScheduleItem");
        await chrome.alarms.clear(this._nameOfLatencyAlarm);
        await chrome.alarms.clear(this._nameOfIntervalAlarm);
        await chrome.alarms.clear(this._nameOfDurationAlarm);
    }

    public async changeScheduleTo(state: ScheduleName) {
        Scheduler._log(`--- ${new Date().toLocaleString()} changeStateTo: ${state.toString()}`);
        await this.stopRunningScheduleItem();
        if (this._schedule && this._schedule[state]) {
            await this.startScheduleItem(this._schedule[state])
        }
    }

    public async run() {
        Scheduler._log("run");
        await this.changeScheduleTo(ScheduleName.Default);
        this.activate();
    }

    public async stop() {
        Scheduler._log("stop");
        await this.stopRunningScheduleItem();
    }

    private static _log(msg: string) {
        console.debug(`===== Scheduler: ${msg}`);
    }
}

export const defaultSchedule: Schedule  = {
    [ScheduleName.Default] : new ScheduleItem(
        600,  // interval = 10 minutes
    ),
    [ScheduleName.AddressCopied]: new ScheduleItem(
        60,  // interval = 1 minute
        1200,  // duration = 20 minutes
        120,  // first run latency = 2 minutes
    ),
    [ScheduleName.BalanceChangePresumed]: new ScheduleItem(
        60,  // interval = 1 minute
        1200,  // duration = 20 minutes
    )
};

export const debugSchedule: Schedule  = {
    [ScheduleName.Default] : new ScheduleItem(
        120,  // interval = 2 minutes
    ),
    [ScheduleName.AddressCopied]: new ScheduleItem(
        60,  // interval = 1 minute
        180,  // duration = 3 minutes
        120,  // first run latency = 2 minutes
    ),
    [ScheduleName.BalanceChangePresumed]: new ScheduleItem(
        60,  // interval = 1 minute
        180,  // duration = 3 minutes
    )
};
