
FontInfo gkt;
unsigned char* glyphImages[96];

int wiggleGenFonts(string fontFile, string outFile, string atlasFile, 
		int SizeX, int SizeY, int Scale, int OffsetX, int OffsetY,
		int PxRange, int doRender)
{

	string enclosingFolder = getEnclosingFolder();
	gkt.sizeX = SizeX;
	gkt.sizeY = SizeY;
	gkt.scale = Scale;
	gkt.offsetX = OffsetX;
	gkt.offsetY = OffsetY;
	gkt.pxRange = PxRange;

	int error = FT_Init_FreeType(&ft);
	if(error) {
		fprintf(stderr, "Error opening freetype\n");
		return 1;
	}

	FT_Face face;
	error = FT_New_Face(ft, fontFile, 0, &face);
	if(error) {
		fprintf(stderr, "Error opening font\n");
		return 1;
	} else {
		printf("[wiggle] Processing font %s\n", fontFile);
	}

	i32 needExtKern = 0;
	if(!FT_HAS_KERNING(face)) {
		needExtKern = 1;
	} 

	printf("Gathering font metrics...\n");
	gkt.lineSpacing = ptf(face->height);
	for(int i = 32; i <= 127; ++i) {
		FT_UInt a = FT_Get_Char_Index(face, i);
		FT_Load_Glyph(face, a, FT_LOAD_NO_SCALE);
		Glyph* t = gkt.glyphs + (i - 32);
		FT_Glyph_Metrics m = face->glyph->metrics;
		t->character = i;
		t->width = ptf(m.width);
		t->height = ptf(m.height);
		t->x = ptf(m.horiBearingX);
		t->y = ptf(m.horiBearingY);
		t->advance = ptf(m.horiAdvance);
	}

	printf("Gathering kerning data...\n");
	if(needExtKern) {
		char buf[4096];
		snprintf(buf, 4096, "%sgetKerningPairsFromOTF.exe %s > %skerning.txt", enclosingFolder, 
				fontFile, enclosingFolder);
		system(buf);
		snprintf(buf, 4096, "%skerning.txt", enclosingFolder);
		char* kernInfo = loadFile(buf);
		isize len = strlen(kernInfo);
		char* text = kernInfo;
		char* end = "\0";
		i64 lines = strtol(text, &end, 10);
		i64 kernScale = 1000;
		text = end;
		while(lines--) {
			i64 a = strtol(text, &end, 10);
			text = end;
			i64 b = strtol(text, &end, 10);
			text = end;
			i64 c = strtol(text, &end, 10);
			text = end;
			gkt.kerning[a-32][b-32] = (f32)c / (f32)kernScale;
		}
	} else {
		for(int i = 32; i <= 127; ++i) {
			FT_UInt a = FT_Get_Char_Index(face, i);
			for(int j = 32; j <= 127; ++j) {
				FT_UInt b = FT_Get_Char_Index(face, j);
				FT_Vector k;
				int error = FT_Get_Kerning(face, a, b, FT_KERNING_UNSCALED, &k);
				if(error) {
					fprintf(stderr, "%d\n", error);
				}

				icvt cc;
				cc.u = k.x;
				double f = (double)cc.i; 
				f /= 2048.0;
				gkt.kerning[i-32][j-32] = (float)f;
			}
		}
	}

	printf("Rendering SDFs...\n");
	if(doRender) {
		char buf[4096];
		snprintf(buf, 4096, "%swiggleTemp", enclosingFolder);
		createdir(buf);
		for(int i = 32; i <= 127; ++i) {
			snprintf(buf, 4096, 
					"start /b %smsdfgen msdf -scale %d -size %d %d "
					"-translate %d %d "
					"-font %s %d -pxrange %d -printmetrics -o %swiggleTemp/w%d.png "
					"> %swiggleTemp/w%dmetrics.txt", 
					enclosingFolder,
					Scale, SizeX, SizeY, OffsetX, OffsetY,
					fontFile, i, PxRange, enclosingFolder, i,
					enclosingFolder, i);
			system(buf);
			printf("\rFinished %d", i - 32);
		}
		printf("\n");
		Sleep(100);
		snprintf(buf, 4096, "%swiggleTemp/w127metrics.txt", enclosingFolder);
		while(!PathFileExists(buf)) {
			Sleep(100);
		}
	} else {
		printf("......Skipping!\n");
	}




	printf("Loading images...\n");
	int atlasSize = 1;
	{
		int w = SizeX; 
		int h = SizeY;
		int wh = w;
		wh += 2;
		double ss = wh * 10;
		int tt = __lzcnt((unsigned int)ss - 1);
		atlasSize = 1 << (31 - tt);
		printf("%d \n", atlasSize);
	}

	PrintedMetrics mm[96];

	{
		char buf[4096];
		for(isize i = 33; i <= 127; ++i) {
			snprintf(buf, 1024, "%swiggleTemp/w%tdmetrics.txt", enclosingFolder, i);
			while(!PathFileExists(buf)) Sleep(100);
			char* text = loadFile(buf);
			
			text = parsePrintedMetrics(text, mm + i-32);
			Glyph* g = gkt.glyphs + i-32;
			g->l = mm[i-32].l;
			g->b = mm[i-32].b;
			g->r = mm[i-32].r;
			g->t = mm[i-32].t;
		}
	}
	/*
	{
		char buf[4096];
		snprintf(buf, 1024, "%swiggleTemp/metrics.txt", enclosingFolder);
		FILE* ff = fopen(buf, "r");
		char* file = NULL;
		if(ff) {
			fseek(ff, 0L, SEEK_END);
			isize size = ftell(ff);
			rewind(ff);
			file = (char*)malloc(size + 64);
			fread(file, 1, size, ff);
			file[size] = '\0';
			fclose(ff);
		}

		if(file != NULL) {
			char* text = file;
			for(int i = 1; i < 96; ++i) {
				text = parsePrintedMetrics(text, mm + i);
				Glyph* g = gkt.glyphs + i;
				g->l = mm[i].l;
				g->b = mm[i].b;
				g->r = mm[i].r;
				g->t = mm[i].t;
			}
		}
	}
	*/

	stbrp_rect* rects = (stbrp_rect*)malloc(sizeof(stbrp_rect) * 100);
	stbrp_node* nodes = (stbrp_node*)malloc(sizeof(stbrp_node) * atlasSize + 2);
	for(int i = 32; i <= 127; ++i) {
		char buf[4096];
		snprintf(buf, 1024, "%swiggleTemp/w%d.png", enclosingFolder, i);
		int w, h, n;
		unsigned char* data = stbi_load(buf, &w, &h, &n, 4);
		glyphImages[i-32] = data;
		float lw = mm[i-32].r - mm[i-32].l;
		float lh = mm[i-32].t - mm[i-32].b;
		lw *= Scale;
		lw += PxRange*2; 
		lh *= Scale;
		lh += PxRange*2;
#ifdef UseFullImage
		lw = SizeX;
#endif 
		lh = SizeY;
		gkt.images[i-32].w = (int)lw;
		gkt.images[i-32].h = (int)lh;
		stbrp_rect* r = rects + (i - 32);
		r->id = i - 32;
		r->w = (int)lw + 2;
		r->h = (int)lh + 2;
		
	}

	printf("Packing atlas...\n");
	{
		stbrp_context ctx = {0};
		stbrp_init_target(&ctx, atlasSize, atlasSize*2, nodes, atlasSize);
		int ret = stbrp_pack_rects(&ctx, rects, 96);
		if(!ret) {
			fprintf(stderr, "Error: packing failed! Increase atlas size and re-run\n");
		}
	}

	printf("Rendering atlas... \n");
	u8* atlas = (u8*)calloc(4, atlasSize * atlasSize * 2);
	int maxHeight = 0;
	for(int i = 1; i < 96; ++i) {
		stbrp_rect* r = rects + i;
		if(r->id > 96 || r->id < 0) continue;
		GlyphImage* g = gkt.images + r->id;
		g->x = r->x + 1;
		g->y = r->y + 1;

		if(glyphImages[i] == NULL) continue;

		float lw = g->w;
		float lh = g->h;
		float lx = (mm[i].l + OffsetX) * Scale;
		float ly = SizeY - (mm[i].t + OffsetY) * Scale;
		lx -= PxRange;
		ly -= PxRange;
		g->bbx = lx;
		g->bby = ly;

#ifdef UseFullImage
		lx = 0;
		lw = SizeX;
#endif  
		ly = 0;
		lh = SizeY;
		wplCopyMemoryBlock(atlas, glyphImages[i], 
				roundf(lx), roundf(ly), (int)lw, (int)lh, SizeX, SizeY,
				g->x, g->y, atlasSize, atlasSize,
				4, 0);

		if(g->y + lh > maxHeight) {
			maxHeight = g->y + lh;
		}
		printf("\rFinished %d ", i);
	}

	printf("\nWriting atlas...\n");
	stbi_write_png(atlasFile, atlasSize, maxHeight + 16, 4, atlas, atlasSize * 4);

	printf("Writing glyph data...\n");
	FILE* fp = fopen(outFile, "wb");
	if(fp) {
		fwrite(&gkt, sizeof(gkt), 1, fp);
		fclose(fp);
	}

	printf("Done! \n");
	printf("%s and %s created\n\n", outFile, atlasFile);
	printf("[FontInfo] %s", outFile);
	printf("SizeX/Y %d %d\nScale %d\nOffsetX/Y %d %d\nPxRange %d\nLineSpacing %d\n",
			gkt.sizeX, gkt.sizeY,
			gkt.scale,
			gkt.offsetX, gkt.offsetY,
			gkt.pxRange,
			gkt.lineSpacing);

	return 0;
}
