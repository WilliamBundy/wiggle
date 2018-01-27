

struct wiggleImg
{
	int w, h, n;
	u8* data;
	int x, y;
	char* file;
};


void wiggleGenAtlas(char* outfile, int size, char** files, int fcount)
{
	wiggleImg* imgFiles = (wiggleImg*)calloc(sizeof(wiggleImg), fcount + 4);
	FontInfo** finfos = (FontInfo**)calloc(sizeof(FontInfo*), fcount + 4);
	stbrp_rect* rects = (stbrp_rect*)calloc(1, sizeof(stbrp_rect) * fcount + 16);
	stbrp_node* nodes = (stbrp_node*)malloc(sizeof(stbrp_node) * size * 2);
	fprintf(stderr, "Loading...\n");
	for(isize i = 0; i < fcount; i += 2) {
		imgFiles[i].file = files[i];
		imgFiles[i].data = stbi_load(files[i], 
				&imgFiles[i].w, 
				&imgFiles[i].h, 
				&imgFiles[i].n, 
				4);
		stbrp_rect* r = rects + i;
		r->id = i;
		r->w = imgFiles[i].w; 
		r->h = imgFiles[i].h;

		//also load font info
		char* fontfile = files[i+1];
		fprintf(stderr, "%s\n", fontfile);
		if(fontfile[0] == '0') continue;
		finfos[i] = loadFontInfo(fontfile);
		gkt = *finfos[r->id];
	}

	fprintf(stderr, "Packing...\n");
	{
		stbrp_context ctx = {0};
		stbrp_init_target(&ctx, size, size, nodes, size);
		int ret = stbrp_pack_rects(&ctx, rects, fcount);
		if(!ret) {
			fprintf(stderr, "Error: packing failed! Increase atlas size and re-run\n");
			return;
		}
	}

	fprintf(stderr, "Rendering...\n");
	u8* atlas = (u8*)calloc(4, size * size);
	int maxHeight = 0;
	for(isize i = 0; i < fcount; i+=2) {
		stbrp_rect* r = rects + i;
		wiggleImg* img = imgFiles + r->id;
		wplCopyMemoryBlock(atlas, img->data,
				0, 0, img->w, img->h, img->w, img->h,
				r->x, r->y, size, size, 4, 0);
		if(r->y+1 + img->h > maxHeight) {
			maxHeight = r->y + img->h;
		}
		printf("%s %d %d\n", img->file, r->x, r->y);
		if(files[i+1][0] == '0') continue;
		finfos[r->id]->atlasX = r->x;
		finfos[r->id]->atlasY = r->y;

		gkt = *finfos[r->id];
		writeFile(files[i+1], finfos[r->id], sizeof(FontInfo));

	}

	fprintf(stderr, "Writing to file...\n");
	stbi_write_png(outfile, size, maxHeight + 16, 4, atlas, size * 4);

}
