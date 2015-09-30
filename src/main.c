
#include <libimg.h>
#include <libimgutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>

int main(int argc, char *argv[]) {

	struct imgImage * nativeImage = NULL;
	struct imgImage * rgbaImage   = NULL;
	struct imgImage * ycbcraImage = NULL;
	struct imgImage * yImage      = NULL;
	struct imgImage * aImage      = NULL;

	if(argc ==3) {

		imgAllocAndRead(&nativeImage, argv[1]);
		imgAllocImage(&ycbcraImage);
		imgAllocImage(&yImage);
		imgAllocImage(&aImage);
		imgAllocImage(&rgbaImage);

		rgbaImage->format = IMG_FMT_RGBA32;
		                    yImage->format = aImage->format = ycbcraImage->format = IMG_FMT_YUV420P;
		rgbaImage->width =  yImage->width  = aImage->width  = ycbcraImage->width  = nativeImage->width;
		rgbaImage->height = yImage->height = aImage->height = ycbcraImage->height = nativeImage->height;

		imgAllocPixelBuffers(ycbcraImage);
		imgAllocPixelBuffers(yImage);
		imgAllocPixelBuffers(aImage);
		imgAllocPixelBuffers(rgbaImage);

		// get data in RGBA32 format.
		imguCopyImage(rgbaImage, nativeImage);

		// switch native to RGB24 format.
		imgFreePixelBuffers(nativeImage);
		nativeImage->format = IMG_FMT_RGB24;
		imgAllocPixelBuffers(nativeImage);

		// Read CbCr Data to final image.
		imguCopyImage(ycbcraImage, rgbaImage);

		// Read Y Image Data.
		imguCopyImage(yImage,      rgbaImage);

		// Read A Image Data.
		{
			uint8_t * src = (uint8_t *)(rgbaImage->data.channel[0]);
			uint8_t * dst = (uint8_t *)(   aImage->data.channel[0]);
			          src += 3;
			for(uint32_t i=0;i<aImage->linearsize[0]; i++) {

				*dst++ = *src;
				src+=4;
			}
		}

		{
			struct SwsContext *ctx = sws_getCachedContext(
				NULL,
				nativeImage->width, // src width
				nativeImage->height, // src height
				AV_PIX_FMT_GRAY8, // src format
				nativeImage->width, // dst width
				nativeImage->height / 2, // dst height
				AV_PIX_FMT_GRAY8, // dst format
				0, // flags??
				NULL,
				NULL,
				NULL);


			// Scale source Y into top half of destination Y
			{
				const uint8_t *const srcSlice[4] = {yImage->data.channel[0], NULL, NULL, NULL};
				const uint8_t *const dst[4] = {ycbcraImage->data.channel[0], NULL, NULL, NULL};

				sws_scale(ctx,
					srcSlice,
					ycbcraImage->linesize, //srcStride,
					0, //srcSliceY,
					nativeImage->height, //srcSliceH,
					dst,
					ycbcraImage->linesize); // dstStride
			}

			// Scale source A into bottom half of destination Y
			{
				const uint8_t *const srcSlice[4] = {aImage->data.channel[0], NULL, NULL, NULL};

				uint8_t * d  = (uint8_t *)ycbcraImage->data.channel[0];
				          d += (ycbcraImage->linesize[0] * (ycbcraImage->height/2));

				const uint8_t *const dst[4] = {d, NULL, NULL, NULL};


				sws_scale(ctx,
					srcSlice,
					ycbcraImage->linesize, //srcStride,
					0, //srcSliceY,
					nativeImage->height, //srcSliceH,
					dst,
					ycbcraImage->linesize); // dstStride
			}

			sws_freeContext(ctx);
		}

		imgFreeAll(yImage);
		imgFreeAll(aImage);
		imgFreeAll(rgbaImage);

		imguCopyImage(nativeImage, ycbcraImage);

		imgFreeAll(ycbcraImage);

		imgWriteFile(nativeImage, argv[2]);

		imgFreeAll(nativeImage);

		return 0;
	}
	return -1;
}

