const l10n = require("../helpers/l10n").default;

export const id = "EVENT_SET_ACTOR_SPRITESHEET_EX";
export const name = "Set actor spritesheet ex";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Set actor spritesheet ex`;
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
    key: "baseTileId",
    label: "Base tile id",
    description: "Base tile id",
    type: "value",
     defaultValue: {
          type: "number",
          value: 0,
        },
  },
  {
    key: "spriteSheetId",
    label: l10n("FIELD_SPRITE_SHEET"),
    description: l10n("FIELD_SPRITE_SHEET_PLAYER_DESC"),
    type: "sprite",
    defaultValue: "LAST_SPRITE",
  },
];

export const compile = (input, helpers) => {
  const { options, _callNative, _stackPush, _stackPushConst, _stackPop, _addComment, _declareLocal, variableSetToScriptValue, setActorId } = helpers;
  const { sprites } = options;
  const sprite = sprites.find((s) => s.id === spriteSheetId);
  if (!sprite) { return; }
  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
    
  setActorId(tmp0, input.actorId);
  variableSetToScriptValue(tmp1, input.baseTileId);
    
  _addComment("Set actor spritesheet ex");
        
  _stackPushConst(`_${sprite.symbol}`);
  _stackPushConst(`___bank_${sprite.symbol}`); 
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("vm_set_actor_spritesheet_ex");
  _stackPop(4);   
  
};
