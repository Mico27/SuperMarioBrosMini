export const id = "EVENT_COPY_BKG_SUBMAP_TO_BKG";
export const name = "Copy scene submap to background";
export const groups = ["EVENT_GROUP_SCREEN"];

export const autoLabel = (fetchArg) => {
  return `Copy scene submap to background`;
};

export const fields = [
  {
    key: "sceneId",
    label: "Scene",
    type: "scene",
	width: "100%",
    defaultValue: "LAST_SCENE",
  },
  {
    key: `source_x`,
    label: "Source X",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `source_y`,
    label: "Source Y",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `dest_x`,
    label: "Destination X",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `dest_y`,
    label: "Destination Y",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "w",
    label: "width",
    description: "width",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: "h",
    label: "height",
    description: "height",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
];

export const compile = (input, helpers) => {
  const { options, _callNative, _stackPushConst, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue } = helpers;
  
  const { scenes } = options;
  const scene = scenes.find((s) => s.id === input.sceneId);
  if (!scene) {
    return;
  }
  
  const tmp0 = _declareLocal("tmp_source_x", 1, true);
  const tmp1 = _declareLocal("tmp_source_y", 1, true);
  const tmp2 = _declareLocal("tmp_dest_x", 1, true);
  const tmp3 = _declareLocal("tmp_dest_y", 1, true);
  const tmp4 = _declareLocal("tmp_w", 1, true);
  const tmp5 = _declareLocal("tmp_h", 1, true);
    
  variableSetToScriptValue(tmp0, input.source_x);
  variableSetToScriptValue(tmp1, input.source_y);
  variableSetToScriptValue(tmp2, input.dest_x);
  variableSetToScriptValue(tmp3, input.dest_y);
  variableSetToScriptValue(tmp4, input.w);
  variableSetToScriptValue(tmp5, input.h);
    
  _addComment("Copy scene submap to background");
  
  _stackPushConst(`_${scene.symbol}`);
  _stackPushConst(`___bank_${scene.symbol}`); 
  _stackPush(tmp5);
  _stackPush(tmp4);
  _stackPush(tmp3);
  _stackPush(tmp2);
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("copy_background_submap_to_background");
  _stackPop(8);  
  
};
