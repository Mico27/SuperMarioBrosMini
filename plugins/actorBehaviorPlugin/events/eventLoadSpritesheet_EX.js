const l10n = require("../helpers/l10n").default;

export const id = "EVENT_LOAD_SPRITESHEET_EX";
export const name = "Set load spritesheet ex";
export const groups = ["EVENT_GROUP_ACTOR"];

export const autoLabel = (fetchArg) => {
  return `Load spritesheet ex`;
};

export const fields = [  
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
   
  variableSetToScriptValue(tmp0, input.baseTileId);
    
  _addComment("Set actor spritesheet ex");
        
  _stackPushConst(`_${sprite.symbol}`);
  _stackPushConst(`___bank_${sprite.symbol}`); 
  _stackPush(tmp0);
  		
  _callNative("vm_load_spritesheet_ex");
  _stackPop(3);   
  
};
