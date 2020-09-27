//==============================================================================
/**
@file       StreamDeckImageManager.cpp
@brief      Loads and stores StreamDeck images
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#include "pch.h"
#include "StreamDeckImageManager.h"
#include "../Vendor/lodepng/lodepng.h"
#include "../Vendor/lodepng/lodepng.cpp"
#include <filesystem>

StreamDeckImageManager::StreamDeckImageManager(const std::string & path)
	:mPath(path)
{
}

/**
	@brief Get the name of all stored images

	@return vector of all stored images
**/
std::vector<std::string> StreamDeckImageManager::getAvailableImages()
{
	std::vector<std::string> imageNames;

	for (const auto& image : mImageNameToBase64Map)
	{
		imageNames.push_back(image.first);
	}

	return imageNames;
}

/**
	@brief Loads all icons into memory

	@return true on success
**/
bool StreamDeckImageManager::loadAllPng()
{
	bool isSuccess = true;

	for (const auto& entry : std::experimental::filesystem::directory_iterator(mPath))
		if (!loadImage(entry.path().filename().string()))
			isSuccess = false;

	return isSuccess;
}

/**
	@brief Loads the image into memory as base64

	@param[in] filename name of the image file

	@return true on success
**/
bool StreamDeckImageManager::loadImage(const std::string& filename)
{
	std::vector<unsigned char> buffer;
	std::string path = "Icons/" + filename;
	if (lodepng::load_file(buffer, path) == 0)
	{
		std::string base64Image;
		imageutils::pngToBase64(base64Image, buffer);
		
		// store to cache
		auto imageIt = mImageNameToBase64Map.find(filename);
		if (imageIt == mImageNameToBase64Map.end())
			mImageNameToBase64Map.insert({ filename, base64Image });
		else
			imageIt->second = base64Image;

		return true;
	}
	return false;
}

/**
	@brief Removes the image from cache

	@param[in] filename name of the image file

	@return true on success
**/
bool StreamDeckImageManager::unloadImage(const std::string& filename)
{
	auto imageIt = mImageNameToBase64Map.find(filename);
	if (imageIt != mImageNameToBase64Map.end())
	{
		mImageNameToBase64Map.erase(imageIt);
		return true;
	}
	return false;
}

/**
	@brief Gets base64 string of image

	@param[in] filename name of the image file

	@return base64 string of image, "" string if error
**/
const std::string StreamDeckImageManager::getImage(const std::string& filename)
{
	auto imageIt = mImageNameToBase64Map.find(filename);
	if (imageIt != mImageNameToBase64Map.end())
		return imageIt->second;
	return "";
}