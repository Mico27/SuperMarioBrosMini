const l10n = require("../helpers/l10n").default;

export const id = "EVENT_GET_ACTOR_BEHAVIOR";
export const name = "Get actor behavior";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Get actor behavior`;
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
    key: "variable",
    label: l10n("FIELD_VARIABLE"),
    description: l10n("FIELD_VARIABLE_DESC"),
    type: "variable",
    defaultValue: "LAST_VARIABLE",
  }
];

export const compile = (input, helpers) => {
  const { _callNative, _stackPush, _stackPop, _addComment, _declareLocal, setActorId, getVariableAlias, _stackPushConst } = helpers;
  
  const tmp0 = _declareLocal("tmp0", 1, true);
    
  setActorId(tmp0, input.actorId);
    
  const variableAlias = getVariableAlias(input.variable);
    
  _addComment("Set actor behavior");
    
  _stackPushConst(variableAlias);
  _stackPush(tmp0);
  		
  _callNative("vm_get_actor_behavior");
  _stackPop(2);   
  
};
