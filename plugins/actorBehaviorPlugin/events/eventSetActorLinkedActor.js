const l10n = require("../helpers/l10n").default;

export const id = "EVENT_SET_ACTOR_LINKED_ACTOR";
export const name = "Set actor linked actor";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Set actor linked actor`;
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
    key: "linkedActorId",
    label: l10n("ACTOR"),
    description: l10n("FIELD_ACTOR_DEACTIVATE_DESC"),
    type: "actor",
    defaultValue: "$self$",
  },
];

export const compile = (input, helpers) => {
  const { _callNative, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue, setActorId } = helpers;
  
  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
    
  setActorId(tmp0, input.actorId);
  setActorId(tmp1, input.linkedActorId);
    
  _addComment("Set actor linked actor");
    
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("vm_set_actor_linked_actor_idx");
  _stackPop(2);   
  
};
