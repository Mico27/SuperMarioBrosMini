export const id = "EVENT_COPY_BKG_SUBMAP_TO_WIN";
export const name = "Copy scene submap to overlay";
export const groups = ["EVENT_GROUP_SCREEN"];

export const autoLabel = (fetchArg) => {
  return `Copy scene submap to overlay`;
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
    key: `bkg_x`,
    label: "Background X",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `bkg_y`,
    label: "Background Y",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `win_x`,
    label: "Overlay X",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `win_y`,
    label: "Overlay Y",
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
  
  const tmp0 = _declareLocal("tmp_bkg_x", 1, true);
  const tmp1 = _declareLocal("tmp_bkg_y", 1, true);
  const tmp2 = _declareLocal("tmp_win_x", 1, true);
  const tmp3 = _declareLocal("tmp_win_y", 1, true);
  const tmp4 = _declareLocal("tmp_w", 1, true);
  const tmp5 = _declareLocal("tmp_h", 1, true);
    
  variableSetToScriptValue(tmp0, input.bkg_x);
  variableSetToScriptValue(tmp1, input.bkg_y);
  variableSetToScriptValue(tmp2, input.win_x);
  variableSetToScriptValue(tmp3, input.win_y);
  variableSetToScriptValue(tmp4, input.w);
  variableSetToScriptValue(tmp5, input.h);
   
  
  _addComment("Copy scene submap to overlay");
  
  _stackPushConst(`_${scene.symbol}`);
  _stackPushConst(`___bank_${scene.symbol}`); 
  _stackPush(tmp5);
  _stackPush(tmp4);
  _stackPush(tmp3);
  _stackPush(tmp2);
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("copy_background_submap_to_overlay");
  _stackPop(8);  
  
};
