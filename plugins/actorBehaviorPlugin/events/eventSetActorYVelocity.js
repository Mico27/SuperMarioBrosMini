const l10n = require("../helpers/l10n").default;

export const id = "EVENT_SET_ACTOR_Y_VELOCITY";
export const name = "Set actor y velocity";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Set actor y velocity`;
};

export const fields = [  
  {
    key: "actorId",
    label: l10n("ACTOR"),
    description: l10n("FIELD_ACTOR_DEACTIVATE_DESC"),
    type: "actor",
    defaultValue: "$self$",
  },
  {
    key: "yVelocity",
    label: "Y velocity",
    description: "Y velocity",
    type: "value",
     defaultValue: {
          type: "number",
          value: 0,
        },
  }
];

export const compile = (input, helpers) => {
  const { _callNative, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue, setActorId } = helpers;
  
  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
    
  setActorId(tmp0, input.actorId);
  variableSetToScriptValue(tmp1, input.yVelocity);
    
  _addComment("Set actor y velocity");
    
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("vm_set_actor_velocity_y");
  _stackPop(2);   
  
};
