export const id = "EVENT_LOAD_META_TILES";
export const name = "Load meta tiles";
export const groups = ["Meta Tiles"];

export const autoLabel = (fetchArg) => {
  return `Load meta tiles`;
};

export const fields = [
  {
    key: "sceneId",
    label: "Metatile Scene",
    type: "scene",
	width: "100%",
    defaultValue: "LAST_SCENE",
  },
];

export const compile = (input, helpers) => {
  const { options, _callNative, _stackPushConst, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue } = helpers;
  
  const { scenes } = options;
  const scene = scenes.find((s) => s.id === input.sceneId);
  if (!scene) {
    return;
  }
    
  _addComment("Load meta tiles");
  
  _stackPushConst(`_${scene.symbol}`);
  _stackPushConst(`___bank_${scene.symbol}`);
  		
  _callNative("vm_load_meta_tiles");
  _stackPop(2);  
  
};
