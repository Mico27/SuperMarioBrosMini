export const id = "EVENT_REPLACE_META_TILE";
export const name = "Replace meta tile";
export const groups = ["Meta Tiles"];

export const autoLabel = (fetchArg) => {
  return `Assign meta tiles`;
};

export const fields = [
  {
    key: `x`,
    label: "X",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `y`,
    label: "Y",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  }, 
  {
    key: `metatile_id`,
    label: "Metatile Id",
    type: "value",
    defaultValue: {
      type: "number",
      value: 0,
    },
  }, 
];

export const compile = (input, helpers) => {
  
  const { _callNative, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue } = helpers;
  
  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
  const tmp2 = _declareLocal("tmp2", 1, true);
    
  variableSetToScriptValue(tmp0, input.x);
  variableSetToScriptValue(tmp1, input.y);
  variableSetToScriptValue(tmp2, input.metatile_id);
    
  _addComment("Replace metatile");
  
  _stackPush(tmp2);
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("vm_replace_meta_tile");
  _stackPop(3);   
};
