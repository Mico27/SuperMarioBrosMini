export const id = "EVENT_COPY_BKG_SUBMAP_TO_BKG_TILESET";
export const name = "Copy scene submap to background tileset";
export const groups = ["EVENT_GROUP_SCREEN"];

export const autoLabel = (fetchArg) => {
  return `Copy scene submap to background tileset`;
};

export const fields = [
{
	type: "group",
	fields: [
		{
			key: "sceneId",
			label: "Scene",
			type: "scene",
			width: "100%",
			defaultValue: "LAST_SCENE",
			conditions: [
			{
				key: "use_far_ptr",
				ne: true
			},
			],
		},
		{
			key: `scene_bank`,
			label: "Scene bank",
			type: "value",
			width: "50%",
			defaultValue: {
			type: "number",
			value: 0,
			},
			conditions: [
			{
				key: "use_far_ptr",
				eq: true
			},
			],
		},
		{
			key: `scene_ptr`,
			label: "Scene Pointer",
			type: "value",
			width: "50%",
			defaultValue: {
			type: "number",
			value: 0,
			},
			conditions: [
			{
				key: "use_far_ptr",
				eq: true
			},
			],
		},
		{
			key: "use_far_ptr",
			label: "Use scene's far ptr",
			type: "checkbox",
			width: "50%",
		},
	]
},
{
	type: "group",
	fields: [
		{
			key: `source_x`,
			label: "Source X",
			type: "value",
			defaultValue: {
			type: "number",
			value: 0,
			},
		},
		{
			key: `source_y`,
			label: "Source Y",
			type: "value",
			defaultValue: {
			type: "number",
			value: 0,
			},
		},
	]
},
{
	type: "group",
	fields: [
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
	]
},
{
	type: "group",
	fields: [
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
	]
},
{
	label: "copy tile attributes to",
	key: "copy_attr",
	type: "select",
	width: "100%",
	options: [
		["none", "None"],
		["background", "Background"],
		["overlay", "Overlay"],
	],
	defaultValue: "none",
	alignBottom: true,
},
{
	type: "group",
	fields: [
		{
			key: `overlay_x`,
			label: "Overlay X",
			type: "value",
			width: "50%",
			defaultValue: {
			type: "number",
			value: 0,
			},
			conditions: [
			{
				key: "copy_attr",
				eq: "overlay",
			},
			],
		},
		{
			key: `overlay_y`,
			label: "Overlay Y",
			type: "value",
			width: "50%",
			defaultValue: {
			type: "number",
			value: 0,
			},
			conditions: [
			{
				key: "copy_attr",
				eq: "overlay",
			},
			],
		},
	]
},
];

export const compile = (input, helpers) => {
  const { options, _callNative, _stackPushConst, _rpn, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue } = helpers;
  
  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
  const tmp2 = _declareLocal("tmp2", 1, true);
  const tmp3 = _declareLocal("tmp3", 1, true);
  const tmp4 = _declareLocal("tmp4", 1, true);
  const tmp5 = _declareLocal("tmp5", 1, true);  
  
  
  _addComment("Copy scene submap to background tileset");
  
  variableSetToScriptValue(tmp0, input.source_x);
  variableSetToScriptValue(tmp1, input.source_y);
   _rpn()
		  .ref(tmp1).int16(256).operator(".MUL")		// (source_y << 8) | source_x
		  .ref(tmp0)        						      
          .operator(".B_OR")
          .refSet(tmp0)
		  .stop();
  
  
  
  
  variableSetToScriptValue(tmp1, input.dest_x);
  variableSetToScriptValue(tmp2, input.dest_y);
  
   _rpn()
		  .ref(tmp2).int16(256).operator(".MUL")		// (dest_y << 8) | dest_x
		  .ref(tmp1)        						      
          .operator(".B_OR")
          .refSet(tmp1)
		  .stop();
  
  variableSetToScriptValue(tmp2, input.w);
  variableSetToScriptValue(tmp3, input.h);  
  
  _rpn()
		  .ref(tmp3).int16(256).operator(".MUL")		// (h << 8) | w
		  .ref(tmp2)        						      
          .operator(".B_OR")
          .refSet(tmp2)
		  .stop();
		  
  if (input.copy_attr === "overlay"){
	variableSetToScriptValue(tmp3, input.overlay_x);
	variableSetToScriptValue(tmp4, input.overlay_y);  
  
	_rpn()
		  .ref(tmp4).int16(256).operator(".MUL")		// (overlay_y << 8) | overlay_x
		  .ref(tmp3)        						      
          .operator(".B_OR")
          .refSet(tmp3)
		  .stop();
	
		  
  }
  
  if (input.use_far_ptr){
	variableSetToScriptValue(tmp4, input.scene_bank);
	variableSetToScriptValue(tmp5, input.scene_ptr);	
  }
  
  
		  
  if (input.use_far_ptr){
	  _stackPush(tmp5);
	  _stackPush(tmp4);
  } else {
	const { scenes } = options;
	const scene = scenes.find((s) => s.id === input.sceneId);
	if (!scene) {
		return;
	}
	_stackPushConst(`_${scene.symbol}`);
	_stackPushConst(`___bank_${scene.symbol}`); 
  }
  
  if (input.copy_attr === "background"){
	  _stackPushConst(1);
  } else if (input.copy_attr === "overlay"){
	  _stackPushConst(2);
  } else {
	  _stackPushConst(0);
  }
  _stackPush(tmp3);
  _stackPush(tmp2);
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("copy_background_submap_to_tileset");
  _stackPop(7);    
  
};
