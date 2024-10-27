export const id = "EVENT_GET_META_TILE_AT_POS";
export const name = "Get meta tile at position";
export const groups = ["Meta Tiles"];

export const autoLabel = (fetchArg) => {
  return `Get meta tile at position`;
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
  	key: "output",
  	label: "Variable",
  	type: "variable",
  	defaultValue: "LAST_VARIABLE",
  },
];

export const compile = (input, helpers) => {
  
  const { _callNative, _stackPushConst, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue, getVariableAlias } = helpers;
  
  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
    
  variableSetToScriptValue(tmp0, input.x);
  variableSetToScriptValue(tmp1, input.y);
  
  const variableAlias = getVariableAlias(input.output);
    
  _addComment("Get metatile at position");
  
  _stackPushConst(variableAlias);
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("vm_get_sram_tile_id_at_pos");
  _stackPop(3);   
};
