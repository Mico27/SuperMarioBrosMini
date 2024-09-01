export const id = "EVENT_SET_PALETTE_COLORS";
export const name = "Set colors of a palette";
export const groups = ["EVENT_GROUP_COLOR"];

export const autoLabel = (fetchArg) => {
  return `Set colors of a palette`;
};

export const fields = [  
  {
    key: "is_gbc",
    label: "Is gameboy color palette",
    type: "checkbox",
    width: "50%",
    defaultValue: true,
  },
  {
    key: "is_sprite",
    label: "Is sprite palette",
    type: "checkbox",
    width: "50%",
    defaultValue: false,
  },
  {
    type: "group",
    conditions: [
      {
        key: "is_gbc",
        ne: true,
      },
	  {
        key: "is_sprite",
        eq: true,
      },
    ],
    fields: [
      {
		key: "palette_idx",
		label: "Palette",
		type: "value",
		width: "100%",
		min: 0,
		max: 1,
		defaultValue: {
			type: "number",
			value: 0,
		},
	  },
    ],
  },
  {
    type: "group",
    conditions: [
      {
        key: "is_gbc",
        eq: true,
      }
    ],
    fields: [
      {
		key: "palette_idx",
		label: "Palette",
		type: "value",
		width: "100%",
		min: 0,
		max: 7,
		defaultValue: {
			type: "number",
			value: 0,
		},
	  },
    ],
  },
  {
    type: "group",
    fields: [
      {
        key: "color0",
        label: "Color 1",
        type: "value",
        defaultValue: {
			type: "number",
			value: 0,
		},
      },
      {
        key: "color1",
        label: "Color 2",
        type: "value",
        defaultValue: {
			type: "number",
			value: 0,
		},
      },
      {
        key: "color2",
        label: "Color 3",
        type: "value",
        defaultValue: {
			type: "number",
			value: 0,
		},
      },
      {
        key: "color3",
        label: "Color 4",
        type: "value",
        defaultValue: {
			type: "number",
			value: 0,
		},
        conditions: [
          {
            key: "is_sprite",
            ne: true
          },
        ],
      },
    ],
  }, 
  {
    type: "group",
    conditions: [
      {
        key: "is_gbc",
        ne: true,
      },
    ],
    width: "50%",
    fields: [
      {
        label: "Note: 0 is white, 1 is light green, 2 is dark green, and 3 is black.",
      },
    ],
  },
];

export const compile = (input, helpers) => {
  const { _callNative, _rpn, _stackPushConst, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue } = helpers;
  const {is_gbc, is_sprite} = input;
  
  
  const tmp_palette_idx = _declareLocal("tmp_palette_idx", 1, true);
  const tmp_colors = [];
  tmp_colors.push(_declareLocal("tmp_color_1", 1, true));
  tmp_colors.push(_declareLocal("tmp_color_2", 1, true));
  tmp_colors.push(_declareLocal("tmp_color_3", 1, true));
  tmp_colors.push(_declareLocal("tmp_color_4", 1, true));
  
  variableSetToScriptValue(tmp_palette_idx, input.palette_idx);
      
  _addComment("Set colors of a palette");
  
  _rpn()  .ref(tmp_palette_idx)  // ((tmp_palette_idx) & 7)
		  .int16((is_sprite)? 8: 0)
          .operator(".B_OR")
          .int16((!is_gbc)? 16: 0)
		  .operator(".B_OR")
		  .refSet(tmp_palette_idx)
          .stop();
		  
    variableSetToScriptValue(tmp_colors[0], input.color0);
	variableSetToScriptValue(tmp_colors[1], input.color1);
	variableSetToScriptValue(tmp_colors[2], input.color2);
	if (!is_sprite){
		variableSetToScriptValue(tmp_colors[3], input.color3);
	}
  
  _stackPush(tmp_colors[3]);
  _stackPush(tmp_colors[2]);
  _stackPush(tmp_colors[1]);
  _stackPush(tmp_colors[0]);
  _stackPush(tmp_palette_idx);
  		
  _callNative("set_palette_colors");
  _stackPop(5);  
  
};
