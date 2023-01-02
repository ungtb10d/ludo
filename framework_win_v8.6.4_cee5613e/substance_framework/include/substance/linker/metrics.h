/*************************************************************************
* ADOBE CONFIDENTIAL
* ___________________ *
*  Copyright 2020-2022 Adobe
*  All Rights Reserved.
* * NOTICE:  All information contained herein is, and remains
* the property of Adobe and its suppliers, if any. The intellectual
* and technical concepts contained herein are proprietary to Adobe
* and its suppliers and are protected by all applicable intellectual
* property laws, including trade secret and copyright laws.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe.
**************************************************************************/

/*
    Defines the SubstanceLinkerHandleMetrics structure. Filled using the
    substanceLinkerHandleGetMetrics function.
*/

#ifndef _SUBSTANCE_LINKER_METRICS_H
#define _SUBSTANCE_LINKER_METRICS_H


/** @brief Substance linker handle metrics description

    Filled using the substanceLinkerHandleGetMetrics function
    declared in handle.h */
typedef struct
{
	/** @brief Total number of nodes currently pushed to the handle */
	unsigned int nodeCount;

	/** @brief Total number of Pixel Processor nodes currently pushed to the handle */
	unsigned int pixProcNodeCount;

	/** @brief Total number of Bitmap nodes currently pushed to the handle */
	unsigned int bitmapNodeCount;

	/** @brief Total number of pixels in the Bitmap nodes currently pushed to the handle */
	unsigned int bitmapPixelCount;

	/** @brief Total number of vertices in the SVG nodes currently pushed to the handle */
	unsigned int svgVerticeCount;

	/** @brief Total number of Uniform Color nodes that are not 1x1 resolution */
	unsigned int non1x1UniformNodeCount;

	/** @brief Total number of nodes that have dynamic output size and increase texture resolution */
	unsigned int dynamicOutputSizeResolutionIncreasingNodeCount;

	/** @brief Total number of Blur nodes that are NOT preceded by SVG nodes */
	unsigned int blurNodesNotPrecededBySVGCount;
} SubstanceLinkerHandleMetrics;


#endif /* ifndef _SUBSTANCE_LINKER_METRICS_H */
