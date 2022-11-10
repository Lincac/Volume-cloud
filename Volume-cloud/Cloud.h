#pragma once

#include"ComputeShader.h"
#include"GenerateTexture.h"
#include"Camera.h"

class Cloud {
public:
	Cloud(int width, int height,float near,float far);
	~Cloud();

	void render(Camera &cam);

	void setSpeed(float newSpeed);
	void setCoverage(float newCoverage);
	void setCrispiness(float newCrispiness);
	void setDensity(float newDensity);
	void setAbsorption(float newAbsorption);
	void setcloudColorTop(glm::vec3 newcloudColorTop);
	void setcloudColorBottom(glm::vec3 newcloudColorBottom);
	void setskyColorTop(glm::vec3 newskyColorTop);
	void setskyColorBottom(glm::vec3 newskyColorBottom);

	unsigned int getCloudTexture() { return cloudMap; };

private:
	ComputeShader perlin_worley, worley, weather,RayMarch;

	static float earthRadius;
	static float sphereInnerRadius;
	static float sphereOuterRadius;

	int width, height;
	unsigned int perlin_worleyMap, worleyMap, weatherMap,cloudMap;

	float near, far;

	float speed;
	float coverage;
	float crispiness; // 卷曲程度
	float curliness; // 卷曲
	float density;
	float absorption;  // 吸收率
	glm::vec3 cloudColorTop;
	glm::vec3 cloudColorBottom;
	glm::vec3 skyColorTop;
	glm::vec3 skyColorBottom;

	glm::mat4 projection;
	glm::mat4 view;

	void initShaders();
	void generateTexture();
};