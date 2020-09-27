//==============================================================================
/**
@file       StreamDeckImageManager.h
@brief      Loads and stores StreamDeck images
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include <map>

#include "ImageUtils.h"

class StreamDeckImageManager
{
public:
	StreamDeckImageManager(const std::string& path);

	bool unloadImage(const std::string& filename);
	const std::string getImage(const std::string& filename);
	bool loadAllPng();

	std::set<std::string> getAvailablePngImages();
	std::set<std::string> getCachedImages();

private:
	// contains cache of base64 images that have been loaded
	std::map<std::string, std::string> mImageNameToBase64Map;

	std::map<std::string, std::string>::iterator loadImage(const std::string& filename);

	const std::string mPath;
};