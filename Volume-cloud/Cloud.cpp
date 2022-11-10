#include"Cloud.h"

#include<GLFW/glfw3.h>

#include<random>
#include<iostream>

glm::vec3 genRandomVec3() {
	std::random_device rd;
	std::mt19937 gen(rd());
	//mt是因为这个伪随机数产生器基于Mersenne Twister算法。
	//19937是因为产生随的机数的周期长，可达到2 ^ 19937 - 1。
	std::uniform_real_distribution<> dis(0.0, 100.0);

	double x, y, z;
	x = dis(gen);
	y = dis(gen);
	z = dis(gen);

	return glm::vec3(x, y, z);
}

float Cloud::earthRadius = 600000;
float Cloud::sphereInnerRadius = 5000;
float Cloud::sphereOuterRadius = 17000;

Cloud::Cloud(int width, int height,float near,float far) :width(width),height(height),near(near),far(far)
{
	absorption = 0.0035;
	coverage = 0.4;
	crispiness = 45;
	curliness = 0.1;
	density = 0.02;
	speed = 450.0;
	cloudColorTop = (glm::vec3(169., 149., 149.) * (1.5f / 255.f));
	cloudColorBottom = (glm::vec3(65., 70., 80.) * (1.5f / 255.f));
	skyColorTop = glm::vec3(0.5, 0.7, 0.8) * 1.05f;
	skyColorBottom = glm::vec3(0.9, 0.9, 0.95) * 1.05f;

	perlin_worleyMap = 0;
	worleyMap = 0;
	weatherMap = 0;
	cloudMap = 0;

	initShaders();
	generateTexture();
}

Cloud::~Cloud() {
	glDeleteTextures(1, &perlin_worleyMap);
	glDeleteTextures(1, &worleyMap);
	glDeleteTextures(1, &weatherMap);

	glDeleteProgram(perlin_worley.ID);
	glDeleteProgram(worley.ID);
	glDeleteProgram(weather.ID);
}

void Cloud::render(Camera& cam) { // const 对象只能调用const函数
	glm::mat4 view = cam.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), (float)width / (float)height, near, far);

	RayMarch.use();
	RayMarch.setVec2("Resolution", glm::vec2(width, height));
	RayMarch.setFloat("absorption", absorption);
	RayMarch.setFloat("coverage", coverage);
	RayMarch.setFloat("crispiness", crispiness);
	RayMarch.setFloat("curliness", curliness);
	RayMarch.setFloat("densityFactor", density);
	RayMarch.setFloat("speed", speed);
	RayMarch.setVec3("lightDirection", glm::normalize(glm::vec3(-.5, 0.5, 1.0)));
	RayMarch.setVec3("lightPosition", glm::normalize(glm::vec3(-.5, 0.5, 1.0)) * 1e6f + cam.Position);
	RayMarch.setVec3("cloudColorTop", cloudColorTop);
	RayMarch.setVec3("cloudColorBottom", cloudColorBottom);
	RayMarch.setVec3("skyColorTop", skyColorTop);
	RayMarch.setVec3("skyColorBottom", skyColorBottom);

	RayMarch.setFloat("earthRadius", earthRadius);
	RayMarch.setFloat("sphereInnerRadius", sphereInnerRadius);
	RayMarch.setFloat("sphereOuterRadius", sphereOuterRadius);

	RayMarch.setFloat("iTime", glfwGetTime());
	RayMarch.setVec3("cameraPosition", cam.Position);
	RayMarch.setMat4("inv_projection", glm::inverse(projection));
	RayMarch.setMat4("inv_view", glm::inverse(view));

	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_3D, perlin_worleyMap);
	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_3D, worleyMap);
	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, weatherMap);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cloudMap);
	glBindImageTexture(0, cloudMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glDispatchCompute(INT_CEIL(width, 16), INT_CEIL(height, 16), 1); 
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Cloud::initShaders() {
	perlin_worley = ComputeShader("shader/perlinworley.comp");
	worley = ComputeShader("shader/worley.comp");
	weather = ComputeShader("shader/weather.comp");

	RayMarch = ComputeShader("shader/RayMarch.comp");
	RayMarch.use();
	RayMarch.setInt("cloud", 11);
	RayMarch.setInt("worley32", 12);
	RayMarch.setInt("weatherTex", 13);
}

void Cloud::generateTexture() {
	if (!perlin_worleyMap) {
		generateTexture3D(128, 128, 128, perlin_worleyMap);
		perlin_worley.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, perlin_worleyMap);
		glBindImageTexture(0, perlin_worleyMap, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
		glDispatchCompute(INT_CEIL(128, 4), INT_CEIL(128, 4), INT_CEIL(128, 4));
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateMipmap(GL_TEXTURE_3D);
	}

	if (!worleyMap) {
		generateTexture3D(32, 32, 32, worleyMap);
		worley.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, worleyMap);
		glBindImageTexture(0, worleyMap, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
		glDispatchCompute(INT_CEIL(32, 4), INT_CEIL(32, 4), INT_CEIL(32, 4));
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateMipmap(GL_TEXTURE_3D);
	}

	if (!weatherMap) {
		generateTexture2D(1024, 1024, weatherMap);
		weather.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, weatherMap);
		glBindImageTexture(0, weatherMap, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
		glDispatchCompute(INT_CEIL(1024, 8), INT_CEIL(1024, 8), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	if (!cloudMap) {
		generateTexture2D(width, height, cloudMap);
	}
}

void Cloud::setSpeed(float newSpeed){
	speed = newSpeed;
}

void Cloud::setCoverage(float newCoverage) {
	coverage = newCoverage;
}

void Cloud::setCrispiness(float newCrispiness) {
	crispiness = newCrispiness;
}

void Cloud::setDensity(float newDensity) {
	density = newDensity;
}

void Cloud::setAbsorption(float newAbsorption) {
	absorption = newAbsorption;
}

void Cloud::setcloudColorTop(glm::vec3 newcloudColorTop) {
	cloudColorTop = newcloudColorTop;
}

void Cloud::setcloudColorBottom(glm::vec3 newcloudColorBottom) {
	cloudColorBottom = newcloudColorBottom;
}

void Cloud::setskyColorTop(glm::vec3 newskyColorTop) {
	skyColorTop = newskyColorTop;
}

void Cloud::setskyColorBottom(glm::vec3 newskyColorBottom) {
	skyColorBottom = newskyColorBottom;
}
