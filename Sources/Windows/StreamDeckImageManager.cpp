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
	@brief Get the name of all png images in directory

	@return set of available png images in directory
**/
std::set<std::string> StreamDeckImageManager::getAvailablePngImages()
{
	std::set<std::string> imageNames;
	for (const auto& entry : std::experimental::filesystem::directory_iterator(mPath))
		if (entry.path().extension().string() == ".png")
			imageNames.insert(entry.path().filename().string());

	return imageNames;
}

/**
	@brief Get the name of all stored images

	@return set of all stored images
**/
std::set<std::string> StreamDeckImageManager::getCachedImages()
{
	std::set<std::string> imageNames;

	for (const auto& image : mImageNameToBase64Map)
		imageNames.insert(image.first);

	return imageNames;
}

/**
	@brief Loads all icons into memory

	@return true on success
**/
bool StreamDeckImageManager::loadAllPng()
{
	bool isSuccess = true;
	std::set<std::string> images = getAvailablePngImages();
	for (const auto& imageName : images)
		if (loadImage(imageName) == mImageNameToBase64Map.end())
			isSuccess = false;

	return isSuccess;
}

/**
	@brief Loads the image into memory as base64

	@param[in] filename name of the image file

	@return iterator to image in cache, 
**/
std::map<std::string, std::string>::iterator StreamDeckImageManager::loadImage(const std::string& filename)
{
	std::vector<unsigned char> buffer;
	std::string path = mPath + filename;
	if (lodepng::load_file(buffer, path) == 0)
	{
		std::string base64Image;
		imageutils::pngToBase64(base64Image, buffer);
		
		// store to cache
		auto imageIt = mImageNameToBase64Map.find(filename);
		if (imageIt == mImageNameToBase64Map.end())
			imageIt = mImageNameToBase64Map.insert({ filename, base64Image }).first;
		else
			imageIt->second = base64Image;

		return imageIt;
	}
	return mImageNameToBase64Map.end();
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
	if (filename.length() > 0)
	{
		auto imageIt = mImageNameToBase64Map.find(filename);
		if (imageIt != mImageNameToBase64Map.end())
			return imageIt->second;
		else
		{
			// if image not cached, try to load it
			imageIt = loadImage(filename);
			if (imageIt != mImageNameToBase64Map.end())
				return imageIt->second;
		}
	}
	return "";
}