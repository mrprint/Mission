TextureInfo the_textures[] = { 
    "skybkg.png", 
	"sprites.png" 
};
SpriteInfo the_sprites[] = {
    { the_textures[0], 0, 0, 1024, 1024, 0, 0, BKG_SIZE / 2, BKG_SIZE / 2 },
    { the_textures[1], 130, 0, 128, 64, 0, 64, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 380, 77, 63, 96, 0, 0, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 445, 77, 65, 97, 63, 0, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 232, 66, 24, 47, 20, 25, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 468, 0, 23, 50, 82, 22, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 0, 0, 128, 128, 0, 0, SPR_SIZE / 2, SPR_SIZE },
    { the_textures[1], 0, 130, 66, 35, 29, 77, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 130, 66, 100, 72, 11, 47, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 364, 0, 102, 75, 18, 44, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 290, 109, 88, 91, 19, 16, SPR_SIZE / 2, SPR_SIZE * 0.75f },
    { the_textures[1], 260, 0, 102, 107, 5, 0, SPR_SIZE / 2, SPR_SIZE * 0.75f }
};
SoundInfo the_sounds[] = { 
    "shot.wav", 
	"hit.wav", 
	"gong.wav" 
};
