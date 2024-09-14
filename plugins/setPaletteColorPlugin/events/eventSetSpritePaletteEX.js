const l10n = require("../helpers/l10n").default;

export const id = "EVENT_PALETTE_SET_SPRITE_EX";
export const name = l10n("EVENT_PALETTE_SET_SPRITE") + " EX";
export const groups = ["EVENT_GROUP_COLOR"];

export const autoLabel = (fetchArg) => {
  return l10n("EVENT_PALETTE_SET_SPRITE") + " EX";
};

export const fields = [
  {
    key: "palette0",
    label: l10n("FIELD_PALETTES"),
    description: l10n("FIELD_PALETTES_DESC"),
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 0,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "palette1",
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 1,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "palette2",
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 2,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "palette3",
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 3,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "palette4",
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 4,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "palette5",
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 5,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "palette6",
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 6,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "palette7",
    type: "palette",
    defaultValue: "keep",
    paletteType: "sprite",
    paletteIndex: 7,
    canKeep: true,
    canRestore: true,
  },
  {
    key: "commit",
    type: "checkbox",
    label: "Commit",
    defaultValue: true,
  },
];

const getPalette = ( palettes, id, fallbackId ) => {
  if (id === "dmg") {
    return DMG_PALETTE;
  }
  return (
    palettes.find((p) => p.id === id) ||
    palettes.find((p) => p.id === fallbackId) ||
    (DMG_PALETTE)
  );
};

export const hexDec = (hex) => parseInt(hex, 16);

export const compile = (input, helpers) => {
  const { options, _paletteLoad, _paletteColor } = helpers;
  const paletteIds = [
		input.palette0,
		input.palette1,
		input.palette2,
		input.palette3,
		input.palette4,
		input.palette5,
		input.palette6,
		input.palette7,
	];
    const { palettes, scene, settings } = options;

    let mask = 0;
    const writePalettes = [];
    for (let i = 0; i < paletteIds.length; i++) {
      const paletteId = paletteIds[i];
      const defaultPaletteId = settings.defaultSpritePaletteIds[i];
      if (paletteId === "keep") {
        continue;
      }
      let palette = getPalette(palettes, paletteId, defaultPaletteId);
      if (paletteId === "restore") {
        // Restore from manual palette
        const scenePaletteId =
          scene.spritePaletteIds[i] ?? settings.defaultSpritePaletteIds[i];
        palette = getPalette(palettes, scenePaletteId, defaultPaletteId);
      }
      mask += 1 << i;
      writePalettes.push(palette);
    }

    if (mask === 0) {
      return;
    }

    _paletteLoad(mask, ".PALETTE_SPRITE", input.commit);

    const parseR = (hex) =>
      Math.floor(hexDec(hex.substring(0, 2)) * (32 / 256));
    const parseG = (hex) =>
      Math.floor(hexDec(hex.substring(2, 4)) * (32 / 256));
    const parseB = (hex) =>
      Math.max(1, Math.floor(hexDec(hex.substring(4, 6)) * (32 / 256)));

    for (const palette of writePalettes) {
      const colors = palette.colors;
      _paletteColor(
        parseR(colors[0]),
        parseG(colors[0]),
        parseB(colors[0]),
        parseR(colors[0]),
        parseG(colors[0]),
        parseB(colors[0]),
        parseR(colors[1]),
        parseG(colors[1]),
        parseB(colors[1]),
        parseR(colors[3]),
        parseG(colors[3]),
        parseB(colors[3])
      );
    }
};
