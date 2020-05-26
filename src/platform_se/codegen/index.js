let fs = require('fs');
let path = require('path');

let prettify = (name, f = ''.toUpperCase) => {
    let c = f.apply(name.charAt(0));
    return (name.toUpperCase() === name || name.toLowerCase() === name)
        ? c + name.slice(1).toLowerCase()
        : c + name.slice(1);
};

const p = path.resolve(__dirname, 'FunctionsDump.txt')
const source = JSON.parse(fs.readFileSync(p));
const tab = '    ';
const ignored = ['TESModPlatform.Add', 'Math'];
const functionNameOverrides = {'getplayer': 'getPlayer'};

let output = `
// Generated automatically. Do not edit.
export declare function printConsole(...arguments: any[]): void;
export declare function writeScript(scriptName: string, src: string): void;
export declare function callNative(className: string, functionName: string, self?: object, ...args: any): any;
export declare function getJsMemoryUsage(): number;
export declare let storage: any;

export declare function on(eventName: 'update', callback: () => void): void;
export declare function once(eventName: 'update', callback: () => void): void;

export declare function on(eventName: 'tick', callback: () => void): void;
export declare function once(eventName: 'tick', callback: () => void): void;

export interface ActivateEvent{
    target: ObjectReference, 
    caster: ObjectReference
}

export interface MoveAttachDetachEvent{
    movedRef: ObjectReference, 
    isCellAttached: boolean
}
export interface WaitStopEvent{ 
    isInterrupted: boolean
}
export interface ObjectLoadedEvent{ 
    object: Form
    isLoaded: boolean
}
export interface LockChangedEvent{ 
    lockedObject: ObjectReference
}

export interface CellFullyLoadedEvent{ 
    cell: Cell
}

export interface GrabReleaseEvent{ 
    refr: ObjectReference
    isGrabbed: boolean
}

export interface SwitchRaceCompleteEvent{ 
    subject: ObjectReference
}

export interface UniqueIDChangeEvent{ 
    oldBaseID: number
    newBaseID: number
}

export interface TrackedStatsEvent{ 
    statName: string
    newValue: number
}

export interface InitScriptEvent{ 
    initializedObject: ObjectReference
}

export interface ResetEvent{ 
    object: ObjectReference
}

export interface CombatEvent{ 
    target: ObjectReference
    actor: ObjectReference
    isCombat: boolean
    isSearching: boolean
}

export interface DeathEvent{ 
    actorDying: ObjectReference
    actorKiller: ObjectReference
}

export interface ContainerChangedEvent{ 
    oldContainer: ObjectReference
    newContainer: ObjectReference
    baseObj: Form
    numItems: number
}

export interface HitEvent{ 
    target: ObjectReference
    agressor: ObjectReference
    source: Form
    projectile: Projectile
    isPowerAttack: boolean
    isSneakAttack: boolean
    isBashAttack: boolean
    isHitBlocked: boolean
}

export interface EquipEvent{ 
    actor: ObjectReference
    baseObj: Form
}

export interface ActiveEffectApplyRemoveEvent{ 
    effect: ActiveMagicEffect
    caster: ObjectReference
    target: ObjectReference
}

export interface MagicEffectApplyEvent{ 
    effect: MagicEffect
    caster: ObjectReference
    target: ObjectReference
}

export declare function on(eventName: 'activate', callback: (even:ActivateEvent) => void): void;
export declare function once(eventName: 'activate', callback: (even:ActivateEvent) => void): void;

export declare function on(eventName: 'waitStop', callback: (even:WaitStopEvent) => void): void;
export declare function once(eventName: 'waitStop', callback: (even:WaitStopEvent) => void): void;

export declare function on(eventName: 'objectLoaded', callback: (even:ObjectLoadedEvent) => void): void;
export declare function once(eventName: 'objectLoaded', callback: (even:ObjectLoadedEvent) => void): void;

export declare function on(eventName: 'moveAttachDetach', callback: (even:MoveAttachDetachEvent) => void): void;
export declare function once(eventName: 'moveAttachDetach', callback: (even:MoveAttachDetachEvent) => void): void;

export declare function on(eventName: 'lockChanged', callback: (even:LockChangedEvent) => void): void;
export declare function once(eventName: 'lockChanged', callback: (even:LockChangedEvent) => void): void;

export declare function on(eventName: 'grabRelease', callback: (even:GrabReleaseEvent) => void): void;
export declare function once(eventName: 'grabRelease', callback: (even:GrabReleaseEvent) => void): void;

export declare function on(eventName: 'cellFullyLoaded', callback: (even:CellFullyLoadedEvent) => void): void;
export declare function once(eventName: 'cellFullyLoaded', callback: (even:CellFullyLoadedEvent) => void): void;

export declare function on(eventName: 'switchRaceComplete', callback: (even:SwitchRaceCompleteEvent) => void): void;
export declare function once(eventName: 'switchRaceComplete', callback: (even:SwitchRaceCompleteEvent) => void): void;

export declare function on(eventName: 'uniqueIdChange', callback: (even:UniqueIDChangeEvent) => void): void;
export declare function once(eventName: 'uniqueIdChange', callback: (even:UniqueIDChangeEvent) => void): void;

export declare function on(eventName: 'trackedStats', callback: (even:TrackedStatsEvent) => void): void;
export declare function once(eventName: 'trackedStats', callback: (even:TrackedStatsEvent) => void): void;

export declare function on(eventName: 'scriptInit', callback: (even:InitScriptEvent) => void): void;
export declare function once(eventName: 'scriptInit', callback: (even:InitScriptEvent) => void): void;

export declare function on(eventName: 'reset', callback: (even:ResetEvent) => void): void;
export declare function once(eventName: 'reset', callback: (even:ResetEvent) => void): void;

export declare function on(eventName: 'combatState', callback: (even:CombatEvent) => void): void;
export declare function once(eventName: 'combatState', callback: (even:CombatEvent) => void): void;

export declare function on(eventName: 'loadGame', callback: () => void): void;
export declare function once(eventName: 'loadGame', callback: () => void): void;

export declare function on(eventName: 'deathEnd', callback: (even:DeathEvent) => void): void;
export declare function once(eventName: 'deathEnd', callback: (even:DeathEvent) => void): void;

export declare function on(eventName: 'deathStart', callback: (even:DeathEvent) => void): void;
export declare function once(eventName: 'deathStart', callback: (even:DeathEvent) => void): void;

export declare function on(eventName: 'containerChanged', callback: (even:ContainerChangedEvent) => void): void;
export declare function once(eventName: 'containerChanged', callback: (even:ContainerChangedEvent) => void): void;

export declare function on(eventName: 'hit', callback: (even:HitEvent) => void): void;
export declare function once(eventName: 'hit', callback: (even:HitEvent) => void): void;

export declare function on(eventName: 'unEquip', callback: (even:EquipEvent) => void): void;
export declare function once(eventName: 'unEquip', callback: (even:EquipEvent) => void): void;

export declare function on(eventName: 'equip', callback: (even:EquipEvent) => void): void;
export declare function once(eventName: 'equip', callback: (even:EquipEvent) => void): void;

export declare function on(eventName: 'magicEffectApply', callback: (even:MagicEffectApplyEvent) => void): void;
export declare function once(eventName: 'magicEffectApply', callback: (even:MagicEffectApplyEvent) => void): void;

export declare function on(eventName: 'effectFinish', callback: (even:ActiveEffectApplyRemoveEvent) => void): void;
export declare function once(eventName: 'effectFinish', callback: (even:ActiveEffectApplyRemoveEvent) => void): void;

export declare function on(eventName: 'effectStart', callback: (even:ActiveEffectApplyRemoveEvent) => void): void;
export declare function once(eventName: 'effectStart', callback: (even:ActiveEffectApplyRemoveEvent) => void): void;

declare class ConsoleComand {
    longName: string;
    shortName: string;
    numArgs: number;
    execute: (...arguments: any[]) => boolean;
}
export declare function findConsoleCommand(cmdName: string): ConsoleComand;

export enum MotionType {
    Dynamic = 1,
    SphereInertia = 2,
    BoxInertia = 3,
    Keyframed = 4,
    Fixed = 5,
    ThinBoxInertia = 6,
    Character = 7
};

export declare namespace SendAnimationEventHook {
    class Context {
        selfId: number;
        animEventName: string;

        storage: Map<string, any>;
    }

    class LeaveContext extends Context {
        animationSucceeded: boolean;
    }

    class Handler {
        enter(ctx: Context);
        leave(ctx: LeaveContext);
    }

    class Target {
        add(handler: Handler)
    }
}
export declare class Hooks {
    sendAnimationEvent: SendAnimationEventHook.Target;
}

export declare let hooks: Hooks;
`;
let dumped = [];

let parseReturnValue = (v) => {
    switch (v.rawType) {
        case 'Int':
        case 'Float':
            return 'number';
        case 'Bool':
            return 'boolean';
        case 'String':
            return 'string';
        case 'IntArray':
        case 'FloatArray':
            return 'number[]';
        case 'BoolArray':
            return 'boolean[]';
        case 'StringArray':
            return 'string[]';
        case 'None':
            return 'void';
        case 'Object':
            return prettify(source.types[v.objectTypeName] ? v.objectTypeName : 'Form');
        case 'ObjectArray':
            return 'object[]';
    }
    throw new Error(`Unknown type ${v.rawType}`);
};

let dumpFunction = (className, f, isGlobal) => {
    if (ignored.includes(className + '.' + f.name)) {
        return;
    }

    let funcName = functionNameOverrides[f.name] || f.name;
    output += tab + `${isGlobal ? 'static ' : ''}${prettify(funcName, ''.toLowerCase)}`;
    output += `(`;
    f.arguments.forEach((arg, i) => {
        let isSetMotioTypeFistArg = funcName.toLowerCase() === "setmotiontype" && i === 0;
        let argType = isSetMotioTypeFistArg ? "MotionType" : parseReturnValue(arg.type);
        output += `${arg.name}: ${argType}`;
        if (i !== f.arguments.length - 1) {
            output += `, `;
        }
    });
    let returnType = parseReturnValue(f.returnType);
    if (f.isLatent) {
        returnType = `Promise<${returnType}>`;
    }

    output += `)`;
    output += `: ${returnType}`;
    output += `;\n`;
};

let dumpType = (data) => {
    if (ignored.includes(data.name) || dumped.includes(data.name)) {
        return;
    }

    if (data.parent) {
        dumpType(data.parent);
    }

    output += `\n// Based on ${prettify(data.name)}.pex\n`;

    output += data.parent
        ? `export declare class ${prettify(data.name)} extends ${prettify(data.parent.name)} {\n`
        : `export declare class ${prettify(data.name)} {\n`;

    output += tab + `static from(form: Form): ${prettify(data.name)};\n`;

    data.memberFunctions.forEach(f => dumpFunction(data.name, f, false));
    data.globalFunctions.forEach(f => dumpFunction(data.name, f, true));

    output += '}\n';

    dumped.push(data.name);
};

if (!source.types.WorldSpace) {
    source.types.WorldSpace = {
        parent: 'Form',
        globalFunctions: [],
        memberFunctions: []
    };
}

for (typeName in source.types) {
    let data = source.types[typeName];
    if (data.parent) {
        data.parent = source.types[data.parent];
    }
    data.name = typeName;
}

for (typeName in source.types) {
    let data = source.types[typeName];
    dumpType(data);
}

fs.writeFileSync('out.ts', output);
