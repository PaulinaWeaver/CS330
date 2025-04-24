///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

// Paulina Weaver
// Professor Enkema
// CS330
// 03-23-2025

#include "SceneManager.h"
#include <GLFW/glfw3.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// free the allocated OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// boolean variable to store the success/failure result of each texture loading call
	bool bReturn = false;

	// load all the required textures into OpenGL memory and assign them a string name identifier
	bReturn = CreateGLTexture(
		"textures/tile.jpg", "floor_tile");

	bReturn = CreateGLTexture(
		"textures/clock.jpg", "clock_face");

	bReturn = CreateGLTexture(
		"textures/wood_clock.jpg", "clock_side");

	bReturn = CreateGLTexture(
		"textures/candle_color.jpg", "candle_side");

	bReturn = CreateGLTexture(
		"textures/candle_top.jpg", "candle_top");

	bReturn = CreateGLTexture(
		"textures/baseboard.png", "baseboard");

	bReturn = CreateGLTexture(
		"textures/stella.png", "wine_lable");

	bReturn = CreateGLTexture(
		"textures/cap.png", "wine_cap");

	bReturn = CreateGLTexture(
		"textures/stripe.png", "chair_cushion");

	bReturn = CreateGLTexture(
		"textures/pasta.jpg", "pasta");

	bReturn = CreateGLTexture(
		"textures/meatball.png", "meatball");

	bReturn = CreateGLTexture(
		"textures/stainless.png", "metal_fork");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method sets up the material properties for each
 *  object in the 3D scene, including color, shininess,
 *  and specular highlights.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f); 
	tileMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f); 
	tileMaterial.shininess = 20.0f; 
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);
	
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	woodMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	woodMaterial.shininess = 60.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL wallMaterial;
	wallMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); 
	wallMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f); 
	wallMaterial.shininess = 20.0f; 
	wallMaterial.tag = "wall";

	m_objectMaterials.push_back(wallMaterial);

	OBJECT_MATERIAL candleMaterial;
	candleMaterial.diffuseColor = glm::vec3(1.0f, 0.95f, 0.85f); 
	candleMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); 
	candleMaterial.shininess = 4.0f; 
	candleMaterial.tag = "candle";

	m_objectMaterials.push_back(candleMaterial);

	OBJECT_MATERIAL flameMaterial;
	flameMaterial.diffuseColor = glm::vec3(1.0f, 0.6f, 0.1f);
	flameMaterial.specularColor = glm::vec3(1.0f, 0.8f, 0.4f);
	flameMaterial.shininess = 80.0f;
	flameMaterial.tag = "flame";

	m_objectMaterials.push_back(flameMaterial);

	OBJECT_MATERIAL baseboardMaterial;
	baseboardMaterial.diffuseColor = glm::vec3(0.85f, 0.85f, 0.85f); 
	baseboardMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.15f); 
	baseboardMaterial.shininess = 40.0f;                           
	baseboardMaterial.tag = "baseboard";

	m_objectMaterials.push_back(baseboardMaterial);

	OBJECT_MATERIAL cushionMaterial;
	cushionMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
	cushionMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);    
	cushionMaterial.shininess = 5.0f;                               
	cushionMaterial.tag = "cushion";

	m_objectMaterials.push_back(cushionMaterial);

	OBJECT_MATERIAL meatballMaterial;
	meatballMaterial.diffuseColor = glm::vec3(0.4f, 0.2f, 0.1f);     
	meatballMaterial.specularColor = glm::vec3(0.1f, 0.05f, 0.03f); 
	meatballMaterial.shininess = 8.0f;                                
	meatballMaterial.tag = "meatball";

	m_objectMaterials.push_back(meatballMaterial);

	OBJECT_MATERIAL plateMaterial;
	plateMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.88f);   
	plateMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  
	plateMaterial.shininess = 12.0f;                             
	plateMaterial.tag = "plate";

	m_objectMaterials.push_back(plateMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	metalMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	metalMaterial.shininess = 64.0f;
	metalMaterial.tag = "fork";

	m_objectMaterials.push_back(metalMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method sets up all the light sources used in the
 *  3D scene. It enables lighting and configures up to four
 *  different light sources
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Turn on lighting in the shader program
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Set up a directional light for soft ambient lighting
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.2f, -0.7f, -0.4f); 
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.15f, 0.15f, 0.15f);   // Gentle base lighting
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.5f, 0.5f, 0.5f);   // Soft fill for surfaces
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.5f, 0.5f, 0.5f);  // Mild highlights
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Set up a point light for overhead warm light
	m_pShaderManager->setVec3Value("pointLights[0].position", -5.0f, 35.0f, 0.5f);    
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.14f, 0.09f, 0.055f);   // Warm ambient
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.75f, 0.5f, 0.3f);       // Gentle warm fill
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.35f, 0.25f, 0.17f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Spot Light - Candle flame
	m_pShaderManager->setVec3Value("spotLight.position", 0.0f, 10.0f, -8.5f);  // Candle flame location
	m_pShaderManager->setVec3Value("spotLight.direction", 0.0f, -0.7f, 0.0f); // Angled down slightly for realistic cast
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.2f, 0.15f, 0.1f);  // Bright ambient flicker for candle
	m_pShaderManager->setVec3Value("spotLight.diffuse", 2.0f, 1.5f, 0.75f); // Bright flame color 
	m_pShaderManager->setVec3Value("spotLight.specular", 2.0f, 1.5f, 0.75f);  // Bright highlights for glossy surfaces
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.03f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.06f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(45.0f))); 
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(55.0f))); 
	m_pShaderManager->setBoolValue("spotLight.bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();

	// Set up material properties for all objects in the scene
	DefineObjectMaterials();

	// Initialize and configure all lighting sources in the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 * Renders the 3D scene by applying transformations and drawing
 * all the objects using basic 3D shapes. This function is
 * responsible for displaying the visual components of the scene
 ***********************************************************/
void SceneManager::RenderScene()
{

	// Candlelight flicker effect using time-based sine wave
	float time = glfwGetTime();  // Get the current time
	float flicker = 0.9f + 0.1f * sin(time * 15.0f);

	// Apply flicker to diffuse and ambient light
	glm::vec3 flickerDiffuse = glm::vec3(0.9f, 0.55f, 0.2f) * flicker;
	glm::vec3 flickerAmbient = glm::vec3(0.08f, 0.05f, 0.02f) * flicker;

	// Update spotlight with flickering light values
	m_pShaderManager->setVec3Value("spotLight.diffuse", flickerDiffuse);
	m_pShaderManager->setVec3Value("spotLight.ambient", flickerAmbient);

	// Render all scene objects
	RenderFloor();
	RenderBackWall();
	RenderRightWall();
	RenderTable();
	RenderClock();
	RenderWineBottle();
	RenderCandle();
	RenderLeftChair();
	RenderRightChair();
	RenderLeftWineGlass();
	RenderRightWineGlass();
	RenderLeftPlate();
	RenderRightPlate();
	RenderLeftFork();
	RenderRightFork();
}

void SceneManager::RenderFloor()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/* Set up the scale, rotation, and position for the floor plane. */
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);  // Scale the floor to fit the scene


	// Set up rotation
	XrotationDegrees = 0.0f;  
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set up position
	positionXYZ = glm::vec3(0.0f, -1.0f, 0.0f);  

	// Apply the transformations to the floor mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture for the floor
	SetShaderTexture("floor_tile");

	// Set the UV scaling for the texture to adjust how it maps on the mesh
	// The texture is stretched with a scale of 2.0 on the U-axis and 1.5 on the V-axis to keep the square shape of the tile
	SetTextureUVScale(2.0, 1.5);

	// Set the floor material
	SetShaderMaterial("tile");

	// Draw the floor mesh with the applied transformations
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::RenderBackWall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/***************************************************************/
	//          WALL                 
	/***************************************************************/

	/* Set up the scale, rotation, and position for the wall. */
	scaleXYZ = glm::vec3(20.0f, 1.0f, 16.0f);  // Wall scale to match the scene

	XrotationDegrees = 90.0f;  // Rotate 90 degrees to make the wall vertical
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position
	positionXYZ = glm::vec3(0.0f, 15.0f, -15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set wall color to light gray
	SetShaderColor(0.914, 0.914, 0.914, 1.0);

	// Set the wall material
	SetShaderMaterial("wall");

	// Draw the wall mesh with the applied transformations
	m_basicMeshes->DrawPlaneMesh();

	/***************************************************************/
	//            BASEBOARD                     
	/***************************************************************/

	// Scale for a thin, long baseboard
	scaleXYZ = glm::vec3(40.0f, 0.1f, 1.5f);

	XrotationDegrees = 90.0f;  // Rotate 90 degrees to make vertical
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position at the bottom of the wall
	positionXYZ = glm::vec3(0.0f, -0.3f, -14.95f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture and UV scaling
	SetShaderTexture("baseboard");
	SetTextureUVScale(10.0, 1.0);

	// Set the baseboard material
	SetShaderMaterial("baseboard");

	// Draw the mesh
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderRightWall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/* Set up the scale, rotation, and position for the wall. */
	scaleXYZ = glm::vec3(15.0f, 1.0f, 16.0f);  // Wall scale to match the scene

	// Set rotations
	XrotationDegrees = 90.0f;  
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position
	positionXYZ = glm::vec3(20.0f, 15.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set wall color to light gray
	SetShaderColor(0.914, 0.914, 0.914, 1.0);

	// Set the wall material
	SetShaderMaterial("wall");

	// Draw the wall mesh with the applied transformations
	m_basicMeshes->DrawPlaneMesh();

	/***************************************************************/
	//                   BASEBOARD                     
	/***************************************************************/

	// Scale for a thin, long baseboard
	scaleXYZ = glm::vec3(30.0f, 0.1f, 1.5f);

	// Set rotations
	XrotationDegrees = 90.0f; 
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// Position at the bottom of the wall
	positionXYZ = glm::vec3(20.0f, -0.3f, 0.0f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture for the baseboard and set UV scaling
	SetShaderTexture("baseboard");
	SetTextureUVScale(10.0, 1.0);

	// Set the baseboard material
	SetShaderMaterial("baseboard");

	// Draw the mesh
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderTable()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/***************************************************************/
	//                   TABLE TOP OVAL                          
	/***************************************************************/

	// Set the scale for the table top to represent an oval
	scaleXYZ = glm::vec3(8.5f, 0.5f, 11.0f);

	// No rotation for the table top
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the table top at a good height
	positionXYZ = glm::vec3(0.0f, 7.0f, -4.0f);

	// Apply the transformations to the table top mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set table color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the table wood material
	SetShaderMaterial("wood");

	// Draw the table top mesh
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	//			TABLE LEG TAPERED CYLINDER FRONT LEFT
	/****************************************************************/

	// Scale for a thin, tall leg
	scaleXYZ = glm::vec3(0.4f, 8.0f, 0.4f);

	// Flip the leg to face the correct direction
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg in the correct spot
	positionXYZ = glm::vec3(-6.0f, 7.0f, 3.0f);

	// Apply the transformations to the table leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set table color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the table wood material
	SetShaderMaterial("wood");

	// Draw the table leg mesh
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//			TABLE LEG TAPERED CYLINDER FRONT RIGHT
	/****************************************************************/

	// Scale for a thin, tall leg
	scaleXYZ = glm::vec3(0.4f, 8.0f, 0.4f);

	// Flip the leg to face the correct direction
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg in the correct spot
	positionXYZ = glm::vec3(6.0f, 7.0f, 3.0f);

	// Apply the transformations to the table leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set table color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the table wood material
	SetShaderMaterial("wood");

	// Draw the table leg mesh
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//			TABLE LEG TAPERED CYLINDER BACK RIGHT
	/****************************************************************/

	// Scale for a thin, tall leg
	scaleXYZ = glm::vec3(0.4f, 8.0f, 0.4f);

	// Flip the leg to face the correct direction
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg in the correct spot
	positionXYZ = glm::vec3(6.0f, 7.0f, -10.2f);

	// Apply the transformations to the table leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set table color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the table wood material
	SetShaderMaterial("wood");

	// Draw the table leg mesh
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//			TABLE LEG TAPERED CYLINDER BACK LEFT
	/****************************************************************/

	// Scale for a thin, tall leg
	scaleXYZ = glm::vec3(0.4f, 8.0f, 0.4f);

	// Flip the leg to face the correct direction
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg in the correct spot
	positionXYZ = glm::vec3(-6.0f, 7.0f, -10.2f);

	// Apply the transformations to the table leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set table color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the table wood material
	SetShaderMaterial("wood");

	// Draw the table leg mesh
	m_basicMeshes->DrawTaperedCylinderMesh();
}

void SceneManager::RenderClock()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Scale the clock
	scaleXYZ = glm::vec3(3.5f, 0.5f, 3.5f);

	// Rotate to align vertically
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position above the table
	positionXYZ = glm::vec3(0.0f, 20.0f, -15.0f);

	// Apply the transformations to the clock mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the clock side texture to the cylindrical body and set UV scaling
	SetShaderTexture("clock_side");
	SetTextureUVScale(15.0, 1.0);

	// Set the clock side material
	SetShaderMaterial("wood");

	// Draw the cylindrical body of the clock (sides only)
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// Apply the clock face texture and adjust UV scaling
	SetShaderTexture("clock_face");
	SetTextureUVScale(1.0, -1.0);

	// Set the clock face material
	SetShaderMaterial("glass");

	// Draw only the front face (top cap) of the clock
	m_basicMeshes->DrawCylinderMesh(true, false, false);
}

void SceneManager::RenderWineBottle()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//				WINE BOTTLE BASE CYLINDER
	/****************************************************************/

	// Set the XYZ scale for the wine bottle's base mesh
	scaleXYZ = glm::vec3(0.6f, 2.5f, 0.6f);

	// Set the rotation angles for the wine bottle base
	XrotationDegrees = 0.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the wine bottle's base to the left of the candle
	positionXYZ = glm::vec3(-2.0f, 7.5f, -6.0f);

	// Apply the transformations to the wine bottle base mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the wine bottle texture and adjust UV scaling
	SetShaderTexture("wine_lable");
	SetTextureUVScale(-1.0, 1.0);

	// Set the bottle material
	SetShaderMaterial("glass");

	// Render the base of the wine bottle (sides only)
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// Set the color of the top and bottom of wine bottle base 
	SetShaderColor(0.129, 0.129, 0.122, 1.0);

	// Set the wine bottle material
	SetShaderMaterial("glass");

	// Render the base of the wine bottle (top & bottom)
	m_basicMeshes->DrawCylinderMesh(true, true, false);

	/****************************************************************/
	//			WINE BOTTLE TOP BASE HALF SPHERE
	/****************************************************************/

	// Set the XYZ scale for the top of the wine bottle to mimic the dome-like shape of a wine bottle’s top
	scaleXYZ = glm::vec3(0.6f, 0.7f, 0.6f);

	// Set the rotation angles for the top mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the half sphere to sit on top of the base cylinder
	positionXYZ = glm::vec3(-2.0f, 10.0f, -6.0f);

	// Apply the transformations to the top mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color of the wine bottle top to a slighty transparent green
	SetShaderColor(0.349, 0.463, 0.114, 0.8);

	// Set the bottle material
	SetShaderMaterial("glass");

	// Render the half-sphere mesh with the transformations applied
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	//				 WINE BOTTLE NECK CYLINDER
	/****************************************************************/

	// Set the XYZ scale for the neck of the wine bottle to create the narrow neck shape
	scaleXYZ = glm::vec3(0.2f, 1.3f, 0.2f);

	// Set the rotation angles for the neck mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the wine bottle’s neck to sit on top of the half sphere
	positionXYZ = glm::vec3(-2.0f, 10.6f, -6.0f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the wine cap texture and adjust UV scaling
	SetShaderTexture("wine_cap");
	SetTextureUVScale(-2.0, 1.0);

	// Set the bottle material
	SetShaderMaterial("glass");

	// Render the cylinder mesh with the transformations applied (sides only)
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// Set the color of the top of wine cap
	SetShaderColor(0.129, 0.176, 0.310, 1.0);

	// Set the wine bottle material
	SetShaderMaterial("glass");

	// Render the cap of the wine bottle (top)
	m_basicMeshes->DrawCylinderMesh(true, false, false);
}

void SceneManager::RenderCandle()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//				CANDLE BASE CYLINDER
	/****************************************************************/

	// Set the XYZ scale for the candle's base mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);

	// Set the rotation angles for the base mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the candle base on the table and towards the back
	positionXYZ = glm::vec3(0.0f, 7.5f, -8.5f);

	// Apply the transformations to the base mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the side texture for the candle base and set UV scaling
	SetShaderTexture("candle_side");
	SetTextureUVScale(2.0, 1.0);

	// Set the candle material
	SetShaderMaterial("candle");

	// Draw the cylindrical body of the candle (sides only)
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// Apply the top texture for the candle top and reset UV scaling
	SetShaderTexture("candle_top");
	SetTextureUVScale(1.0, 1.0);

	// Set the candle material
	SetShaderMaterial("candle");

	// Draw only the top cap of the candle
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	/****************************************************************/
	//					CANDLE FLAME CONE
	/****************************************************************/

	// Set the XYZ scale for the flame
	scaleXYZ = glm::vec3(0.07f, 0.4f, 0.07f);

	// Set the XYZ rotation for the flame
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the flame to sit on top of the candle base
	positionXYZ = glm::vec3(0.0f, 9.5f, -8.5f);

	// Apply transformations to the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set flame color
	SetShaderColor(1.0, 0.576, 0.161, 1.0);

	// Set the flame material
	SetShaderMaterial("flame");

	// Draw the cone mesh to represent the candle flame
	m_basicMeshes->DrawConeMesh();
}

void SceneManager::RenderLeftChair()
{ 
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/***************************************************************/
	//         CHAIR SEAT                          
	/***************************************************************/

	// Set the scale for the chair seat
	scaleXYZ = glm::vec3(6.3f, 0.5f, 6.0f);

	// Set rotation 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position
	positionXYZ = glm::vec3(-7.2f, 3.5f, -4.0f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set to wood material
	SetShaderMaterial("wood");

	// Draw the chair seat
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************/
	//         CHAIR CUSHION                          
	/***************************************************************/

	// Set the scale for the cushion
	scaleXYZ = glm::vec3(5.8f, 0.5f, 5.8f);

	// Set rotations 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position on top of the chair seat
	positionXYZ = glm::vec3(-7.0f, 4.0f, -4.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture and set UV scaling
	SetShaderTexture("chair_cushion");
	SetTextureUVScale(10.0, 10.0);

	// Set to cushion material
	SetShaderMaterial("cushion");

	// Draw the cushion
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG FRONT LEFT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(-10.1f, 1.3f, -1.25f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG FRONT RIGHT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(-4.3f, 1.3f, -1.25f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG BACK LEFT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(-10.1f, 1.3f, -6.75f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG BACK RIGHT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(-4.3f, 1.3f, -6.75f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR BACK POST 1
	/****************************************************************/

	// Set the scale for the back post
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;  // Slight angle for comfort

	// Position the back post
	positionXYZ = glm::vec3(-10.75f, 7.5f, -6.75f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the back post using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR BACK POST 2
	/****************************************************************/

	// Set the scale for the back post
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;  // Slight angle

	// Position the back post
	positionXYZ = glm::vec3(-10.75f, 7.5f, -1.25f);


	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the back post using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 1
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;  // Slight angle

	// Position the slat correctly 
	positionXYZ = glm::vec3(-11.25f, 10.5f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 2
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;  // Slight angle

	// Position the slat correctly
	positionXYZ = glm::vec3(-11.0f, 9.0f, -4.0f);


	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 3
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;  // Slight angle

	// Position the slat correctly
	positionXYZ = glm::vec3(-10.75f, 7.5f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 4
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;  // Slight angle

	// Position the slat correctly 
	positionXYZ = glm::vec3(-10.5f, 6.0f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 5
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;  // Slight angle

	// Position the slat correctly 
	positionXYZ = glm::vec3(-10.25f, 4.5f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderRightChair()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/***************************************************************/
	//         CHAIR SEAT                          
	/***************************************************************/

	// Set the scale for the chair seat
	scaleXYZ = glm::vec3(6.3f, 0.5f, 6.0f);

	// Set rotations
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position
	positionXYZ = glm::vec3(7.2f, 3.5f, -4.0f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set to wood material
	SetShaderMaterial("wood");

	// Draw the chair seat
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************/
	//         CHAIR CUSHION                          
	/***************************************************************/

	// Set the scale for the cushion
	scaleXYZ = glm::vec3(5.8f, 0.5f, 5.8f);

	// Set rotations
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the cushion on top of the seat
	positionXYZ = glm::vec3(7.0f, 4.0f, -4.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture and set UV scaling
	SetShaderTexture("chair_cushion");
	SetTextureUVScale(10.0, 10.0);

	// Set the cushion material
	SetShaderMaterial("cushion");

	// Draw the cushion
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG FRONT RIGHT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(10.1f, 1.3f, -1.25f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG FRONT LEFT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(4.3f, 1.3f, -1.25f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG BACK RIGHT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(10.1f, 1.3f, -6.75f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR LEG BACK LEFT
	/****************************************************************/

	// Set the scale for a thin chair leg
	scaleXYZ = glm::vec3(0.5f, 4.5f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the leg correctly under the seat
	positionXYZ = glm::vec3(4.3f, 1.3f, -6.75f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the chair leg using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR BACK POST 1
	/****************************************************************/

	// Set the scale for the back post
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;  // Slight angle

	// Position the back post correctly
	positionXYZ = glm::vec3(10.75f, 7.5f, -6.75f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the back post using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			CHAIR BACK POST 2
	/****************************************************************/

	// Set the scale for the back post
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;  // Slight angle

	// Position the back post correctly 
	positionXYZ = glm::vec3(10.75f, 7.5f, -1.25f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the back post using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 1
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;  // Slight angle

	// Position the slat correctly 
	positionXYZ = glm::vec3(11.25f, 10.5f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 2
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;  // Slight angle

	// Position the slat correctly
	positionXYZ = glm::vec3(11.0f, 9.0f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 3
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;  // Slight angle

	// Position the slat correctly
	positionXYZ = glm::vec3(10.75f, 7.5f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 4
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;  // Slight angle

	// Position the slat correctly under the seat
	positionXYZ = glm::vec3(10.5f, 6.0f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//			SLAT 5
	/****************************************************************/

	// Set the scale for the slat
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.5f);

	// Set rotation angles
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -10.0f;  // Slight angle

	// Position the slat correctly 
	positionXYZ = glm::vec3(10.25f, 4.5f, -4.0f);

	// Apply scaling, rotation, and position transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set chair color
	SetShaderColor(0.18, 0.12, 0.09, 1.0);

	// Set the material to wood
	SetShaderMaterial("wood");

	// Render the slat using a box mesh
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderLeftWineGlass()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//			BASE CYLINDER
	/****************************************************************/

	// Set scale for the base
	scaleXYZ = glm::vec3(0.5f, 0.03f, 0.5f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-2.9f, 7.5f, -1.5f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the base of the wine glass
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	//		 BASE TAPERED CYLINDER 
	/****************************************************************/

	// Set scale for the base tapered cylinder
	scaleXYZ = glm::vec3(0.15f, 0.3f, 0.15f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-2.9f, 7.5f, -1.5f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the base tapered cylinder of the wine glass
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	/****************************************************************/
	//		 STEM CYLINDER	
	/****************************************************************/

	// Set scale for the stem
	scaleXYZ = glm::vec3(0.07f, 1.0f, 0.07f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-2.9f, 7.8f, -1.5f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.4);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the stem of the wine glass
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/****************************************************************/
	//		STEM TAPERED CYLINDER
	/****************************************************************/

	// Set scale for the stem tapered cylinder
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);

	// Set the rotation angles 
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-2.9f, 8.95f, -1.5f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the stem tapered cylinder part of the wine glass
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	/****************************************************************/
	//		WINE HALF SPHERE
	/****************************************************************/

	// Set scale for the wine 
	scaleXYZ = glm::vec3(0.6f, 0.8f, 0.6f);

	// Set the rotation angles 
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-2.9f, 9.72f, -1.5f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the wine color
	SetShaderColor(0.2, 0.0, 0.1, 0.9);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the wine part of the wine glass
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	//		BOWL TAPERED CYLINDER
	/****************************************************************/

	// Set scale for the bowl tapered cylinder
	scaleXYZ = glm::vec3(0.6f, 0.8f, 0.6f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-2.9f, 9.72f, -1.5f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the bowl part of the wine glass
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
}

void SceneManager::RenderRightWineGlass()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//		BASE CYLINDER	
	/****************************************************************/

	// Set scale for the base
	scaleXYZ = glm::vec3(0.5f, 0.03f, 0.5f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(3.3f, 7.5f, -6.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the base of the wine glass
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	//		 BASE TAPERED CYLINDER	
	/****************************************************************/

	// Set scale for the base tapered cylinder
	scaleXYZ = glm::vec3(0.15f, 0.3f, 0.15f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(3.3f, 7.5f, -6.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the base tapered cylinder part of the wine glass
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	/****************************************************************/
	//		STEM CYLINDER	
	/****************************************************************/

	// Set scale for the stem
	scaleXYZ = glm::vec3(0.07f, 1.0f, 0.07f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(3.3f, 7.8f, -6.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.4);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the stem of the wine glass
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/****************************************************************/
	// STEM TAPERED CYLINDER
	/****************************************************************/

	// Set scale for the stem tapered cylinder
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);

	// Set the rotation angles 
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(3.3f, 8.95f, -6.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the stem tapered cylinder part of the wine glass
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	/****************************************************************/
	// WINE HALF SPHERE
	/****************************************************************/

	// Set scale for the wine 
	scaleXYZ = glm::vec3(0.6f, 0.8f, 0.6f);

	// Set the rotation angles 
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(3.3f, 9.72f, -6.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the wine color
	SetShaderColor(0.2, 0.0, 0.1, 0.9);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the wine part of the wine glass
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	// BOWL TAPERED CYLINDER
	/****************************************************************/

	// Set scale for the bowl tapered cylinder
	scaleXYZ = glm::vec3(0.6f, 0.8f, 0.6f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(3.3f, 9.72f, -6.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// Set the glass material
	SetShaderMaterial("glass");

	// Render the bowl tapered cylinder part of the wine glass
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
}

void SceneManager::RenderLeftPlate()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//		PLATE
	/****************************************************************/

	// Set scale for the plate
	scaleXYZ = glm::vec3(2.0f, 0.1f, 2.0f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the plate
	positionXYZ = glm::vec3(-6.0f, 7.5f, -3.5f);

	// Apply the transformations to the plate
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color (sides and bottom)
	SetShaderColor(0.898f, 0.902f, 0.910f, 1.0);

	// Set the plate material
	SetShaderMaterial("plate");

	// Render the plate (sides and bottom)
	m_basicMeshes->DrawCylinderMesh(false, true, true);

	// Texture top of plate
	SetShaderTexture("pasta");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("plate");

	// Render the top of the plate
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	/****************************************************************/
	//				MEATBALL 1
	/****************************************************************/

	// Set scale for the meatball
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-6.5f, 7.7f, -3.5f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the meatball texture and adjust UV scaling
	SetShaderTexture("meatball");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("meatball");

	// Render the meatball
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/
	//				MEATBALL 2
	/****************************************************************/

	// Set scale for the meatball
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-5.8f, 7.7f, -3.9f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the meatball texture and adjust UV scaling
	SetShaderTexture("meatball");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("meatball");

	// Render the meatball
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/
	//				MEATBALL 3
	/****************************************************************/

	// Set scale for the meatball
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-5.8f, 7.7f, -3.2f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the meatball texture and adjust UV scaling
	SetShaderTexture("meatball");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("meatball");

	// Render the meatball
	m_basicMeshes->DrawSphereMesh();
}

void SceneManager::RenderRightPlate()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//		PLATE
	/****************************************************************/

	// Set scale for the plate
	scaleXYZ = glm::vec3(2.0f, 0.1f, 2.0f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the plate
	positionXYZ = glm::vec3(6.0f, 7.5f, -3.5f);

	// Apply the transformations to the plate
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color (sides and bottom)
	SetShaderColor(0.898f, 0.902f, 0.910f, 1.0);

	// Set the plate material
	SetShaderMaterial("plate");

	// Render the plate (sides and bottom)
	m_basicMeshes->DrawCylinderMesh(false, true, true);

	// Texture top of plate
	SetShaderTexture("pasta");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("plate");

	// Render the top of the plate
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	/****************************************************************/
	//				MEATBALL 1
	/****************************************************************/

	// Set scale for the meatball
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(6.5f, 7.7f, -3.5f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the meatball texture and adjust UV scaling
	SetShaderTexture("meatball");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("meatball");

	// Render the meatball
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/
	//				MEATBALL 2
	/****************************************************************/

	// Set scale for the meatball
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(5.8f, 7.7f, -3.9f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the meatball texture and adjust UV scaling
	SetShaderTexture("meatball");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("meatball");

	// Render the meatball
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/
	//				MEATBALL 3
	/****************************************************************/

	// Set scale for the meatball
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(5.8f, 7.7f, -3.2f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the meatball texture and adjust UV scaling
	SetShaderTexture("meatball");
	SetTextureUVScale(1.0, 1.0);

	// Set the material
	SetShaderMaterial("meatball");

	// Render the meatball
	m_basicMeshes->DrawSphereMesh();
}

void SceneManager::RenderLeftFork()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//		HANDLE
	/****************************************************************/

	// Set scale for the handle
	scaleXYZ = glm::vec3(0.15f, 1.25f, 0.1f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-6.5f, 7.5f, -1.0f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(0.5, 4.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the handle
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//		FORK ROOT
	/****************************************************************/

	// Set scale for the root
	scaleXYZ = glm::vec3(0.5f, 0.105f, 0.6f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 180.0f;

	// Set the position 
	positionXYZ = glm::vec3(-5.9f, 7.5f, -1.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the root
	m_basicMeshes->DrawPrismMesh();

	/****************************************************************/
	//		PRONG 1
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-5.6f, 7.51f, -1.2f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//		PRONG 2
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-5.6f, 7.51f, -1.07f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//		PRONG 3
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the prong
	positionXYZ = glm::vec3(-5.6f, 7.51f, -0.94f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//		PRONG 4
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(-5.6f, 7.51f, -0.8f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();
}

void SceneManager::RenderRightFork()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	//		HANDLE
	/****************************************************************/

	// Set scale for the handle
	scaleXYZ = glm::vec3(0.15f, 1.25f, 0.1f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(6.5f, 7.5f, -6.0f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(0.5, 4.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the handle
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//		FORK ROOT
	/****************************************************************/

	// Set scale for the root
	scaleXYZ = glm::vec3(0.5f, 0.105f, 0.6f);

	// Set the rotation angles 
	XrotationDegrees = 0.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 180.0f;

	// Set the position 
	positionXYZ = glm::vec3(5.9f, 7.5f, -6.0f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the root
	m_basicMeshes->DrawPrismMesh();

	/****************************************************************/
	//		PRONG 1
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(5.6f, 7.51f, -6.2f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//		PRONG 2
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(5.6f, 7.51f, -6.07f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//		PRONG 3
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position of the prong
	positionXYZ = glm::vec3(5.6f, 7.51f, -5.94f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//		PRONG 4
	/****************************************************************/

	// Set scale for the prong
	scaleXYZ = glm::vec3(0.04f, 0.45f, 0.03f);

	// Set the rotation angles 
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// Set the position 
	positionXYZ = glm::vec3(5.6f, 7.51f, -5.8f);

	// Apply the transformations 
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Texture the fork
	SetShaderTexture("metal_fork");
	SetTextureUVScale(1.0, 1.0);

	// Set the fork material
	SetShaderMaterial("fork");

	// Render the prong
	m_basicMeshes->DrawTaperedCylinderMesh();
}

