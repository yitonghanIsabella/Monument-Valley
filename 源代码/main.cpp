#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"
#include "Shader.h"
#include "Camera.h"

#include <vector>
#include <string>

int WIDTH = 1400;
int HEIGHT = 1400;

int mainWindow;

Camera* cameraSky = new Camera();
Camera* camera = new Camera();
Light* light = new Light();
MeshPainter* painter = new MeshPainter();

std::vector<TriMesh*> meshList;

class MatrixStack {
	int		_index;
	int		_size;
	glm::mat4* _matrices;

public:
	MatrixStack(int numMatrices = 100) :_index(0), _size(numMatrices)
	{
		_matrices = new glm::mat4[numMatrices];
	}

	~MatrixStack()
	{
		delete[]_matrices;
	}

	void push(const glm::mat4& m) {
		assert(_index + 1 < _size);
		_matrices[_index++] = m;
	}

	glm::mat4& pop() {
		assert(_index - 1 >= 0);
		_index--;
		return _matrices[_index];
	}
};

bool isMoving = false;	//前进
bool Climb = false;		//爬楼梯
bool Down = false;		//滑滑梯
bool Goodbye = false;	//挥手

bool Elevator = false;  //电梯
bool Bridge = false;	//吊桥
bool Slide = false;		//滑梯

int Time = 0;

//小人相关参数
float ratio = 0.15;		//小人放缩比例
int direction = 3;		//小人前进方向
float step = 0.5;		//小人步伐大小
float xDown = 0;		//小人滑滑梯参数
float yDown = 0;
glm::vec3 Move = glm::vec3(0.0, 0.0, 0.0);	//小人移动参数

//建筑物相关参数
float yElevator = 0;	//电梯上升参数
int bridge = 90;		//吊桥旋转参数
float firstStair = 0;	//台阶上升参数
float up = 0;
int slide = 90;			//滑梯旋转参数

struct Robot
{
	float TORSO_HEIGHT = 2.5 * ratio;
	float TORSO_WIDTH = 3.4 * ratio;
	float HEAD_HEIGHT = 1.8 * ratio;
	float ARM_HEIGHT = 0.7 * ratio;
	float LEG_HEIGHT = 0.8 * ratio;
	float ARM_WIDTH = 0.4 * ratio;
	float LEG_WIDTH = 0.5 * ratio;
	float head = 2.0 * ratio;	
	float shoulder = 0.6 * ratio;
	float HAT_R = 1.0 * ratio;
	float HAT_H = 1.2 * ratio;

	// 关节角和菜单选项值
	enum {
		Torso,			// 躯干
		Head,			// 头部
		Hat,			// 帽子
		RightShoulder,	// 右肩膀
		RightUpperArm,	// 右大臂
		RightLowerArm,	// 右小臂
		LeftShoulder,	// 左肩膀
		LeftUpperArm,	// 左大臂
		LeftLowerArm,	// 左小臂
		RightUpperLeg,	// 右大腿
		RightLowerLeg,	// 右小腿
		LeftUpperLeg,	// 左大腿
		LeftLowerLeg,	// 左小腿
	};

	// 关节角大小
	GLfloat theta[13] = {
		0.0,    // Torso
		0.0,    // Head
		0.0,	// RightShoulder
		0.0,    // RightUpperArm
		0.0,    // RightLowerArm
		0.0,    // LeftShoulder
		0.0,    // LeftUpperArm
		0.0,    // LeftLowerArm
		0.0,    // RightUpperLeg
		0.0,    // RightLowerLeg
		0.0,    // LeftUpperLeg
		0.0     // LeftLowerLeg
	};
};

Robot robot;
int Selected_mesh = robot.Torso;
TriMesh* Torso = new TriMesh();
TriMesh* Head = new TriMesh();
TriMesh* Hat = new TriMesh();
TriMesh* Shoulder = new TriMesh();
TriMesh* Arm = new TriMesh();
TriMesh* Leg = new TriMesh();
TriMesh* cube = new TriMesh();
TriMesh* cone = new TriMesh();
TriMesh* disk = new TriMesh();

openGLObject TorsoObject;
openGLObject HeadObject;
openGLObject HatObject;
openGLObject ShoulderObject;
openGLObject ArmObject;
openGLObject LegObject;

void init()
{
	std::string vshader, fshader, fshaderTex, fshaderLightTex, vshaderSky, fshaderSky;
	vshader = "shaders/vshader.glsl";
	fshader = "shaders/fshader.glsl";
	fshaderTex = "shaders/fshaderTex.glsl";
	fshaderLightTex = "shaders/fshaderLightTex.glsl";
	vshaderSky = "shaders/vshaderSky.glsl";
	fshaderSky = "shaders/fshaderSky.glsl";

	//设置相机
	camera->setR(26);
	cameraSky->setR(20);

	//设置光源
	light->setTranslation(glm::vec3(-2.0, 30.0, 2.0));		// 位置
	light->setAmbient(glm::vec4(0.797, 0.77, 0.797, 1.0));  // 环境光
	light->setDiffuse(glm::vec4(0.4, 0.4, 0.4, 1.0));		// 漫反射
	light->setSpecular(glm::vec4(1.0, 1.0, 1.0, 1.0));		// 镜面反射
	light->setShininess(1);

	//设置小人
	Torso->generateCone(100, robot.TORSO_WIDTH / 2, robot.TORSO_HEIGHT);
	Head->readOff("./assets/off/sphere.off");
	Hat->generateCone(100, robot.HAT_R, robot.HAT_H);
	Shoulder->readOff("./assets/off/sphere.off");
	Arm->generateCylinder(100, robot.ARM_WIDTH / 2, robot.ARM_HEIGHT);
	Leg->generateCylinder(100, robot.LEG_WIDTH / 2, robot.LEG_HEIGHT);

	painter->bindObjectAndData(Torso, TorsoObject, "./assets/2.jpg", vshader, fshaderLightTex);
	painter->bindObjectAndData(Head, HeadObject, "", vshader, fshader);
	painter->bindObjectAndData(Hat, HatObject, "./assets/2.jpg", vshader, fshaderLightTex);
	painter->bindObjectAndData(Shoulder, ShoulderObject, "", vshader, fshader);
	painter->bindObjectAndData(Arm, ArmObject, "", vshader, fshader);
	painter->bindObjectAndData(Leg, LegObject, "", vshader, fshader);
	painter->bindObjectAndDataSky();

	//设置建筑物
	cube->generateCube();
	cube->setScale(glm::vec3(0.4, 3, 0.4));
	cube->setTranslation(glm::vec3(2, -2.85, 4));
	painter->addMesh(cube, "cube", "./assets/1.jpg", vshader, fshaderLightTex);
	meshList.push_back(cube);
	
	cone->generateCone(70, 0.2, 0.2);
	cone->setScale(glm::vec3(3.5, 8, 3.5));
	cone->setTranslation(glm::vec3(-2.8, 7.6, 4));
	painter->addMesh(cone, "cone", "./assets/1.jpg", vshader, fshaderLightTex);
	meshList.push_back(cone);

	disk->generateDisk(70, 0.235);
	disk->setTranslation(glm::vec3(-1.81, 0.15, 3.11));
	painter->addMesh(disk, "disk", "./assets/1.jpg", vshader, fshaderLightTex);
	meshList.push_back(disk);
	
	glClearColor(0.39, 0.85, 1.0, 1.0);
}

//前进一步
void MoveTo() {
	if (direction == 1)
		Move[2] -= step;
	else if (direction == 2)
		Move[0] += step;
	else if (direction == 3)
		Move[2] += step;
	else if (direction == 4)
		Move[0] -= step;
}

//抬起手臂
void liftArm() {
	robot.theta[robot.LeftUpperArm] = -120;
}

//放下手臂
void downArm() {
	robot.theta[robot.LeftUpperArm] = 0;
	robot.theta[robot.LeftLowerArm] = 0;
}

//挥手
void goodbye1() {
	robot.theta[robot.LeftLowerArm] = -20;
}
void goodbye2() {
	robot.theta[robot.LeftLowerArm] = -60;
}

//坐下
void sit() {
	robot.theta[robot.RightUpperLeg] = -28;
	robot.theta[robot.LeftUpperLeg] = -28;
	robot.theta[robot.RightUpperArm] = 28;
	robot.theta[robot.LeftUpperArm] = -28;
	robot.theta[robot.RightShoulder] = 0;
	robot.theta[robot.LeftShoulder] = 0;
	Move.y -= 0.4;
}

//爬台阶
void climb() {
	robot.theta[robot.RightUpperLeg] = -35;
	robot.theta[robot.RightLowerLeg] = 45;
	robot.theta[robot.LeftUpperLeg] = 15;
	robot.theta[robot.LeftLowerLeg] = 15;
	robot.theta[robot.RightShoulder] = 15;
	robot.theta[robot.LeftShoulder] = -15;
	MoveTo();
	Move.y += 0.2;
}
void climb1() {
	robot.theta[robot.RightUpperLeg] = -35;
	robot.theta[robot.RightLowerLeg] = 45;
	robot.theta[robot.LeftUpperLeg] = 15;
	robot.theta[robot.LeftLowerLeg] = 15;
	robot.theta[robot.RightShoulder] = 15;
	robot.theta[robot.LeftShoulder] = -15;
	Move[0] += 0.11;
	Move[2] -= 0.69;
	Move.y += 0.272;
}
void climb2() {
	robot.theta[robot.LeftUpperLeg] = -35;
	robot.theta[robot.LeftLowerLeg] = 45;
	robot.theta[robot.RightUpperLeg] = 15;
	robot.theta[robot.RightLowerLeg] = 15;
	robot.theta[robot.LeftShoulder] = 15;
	robot.theta[robot.RightShoulder] = -15;
	Move[0] += 0.11;
	Move[2] -= 0.69;
	Move.y += 0.272;
}

//走路
void walk1() {
	robot.theta[robot.RightUpperLeg] = 10;
	robot.theta[robot.LeftUpperLeg] = -10;
	robot.theta[robot.RightLowerLeg] = 15;
	robot.theta[robot.LeftLowerLeg] = 15;
	robot.theta[robot.RightShoulder] = -15;
	robot.theta[robot.LeftShoulder] = 15;
	MoveTo();
}
void walk2() {
	robot.theta[robot.LeftUpperLeg] = 10;
	robot.theta[robot.RightUpperLeg] = -10;
	robot.theta[robot.LeftLowerLeg] = 15;
	robot.theta[robot.RightLowerLeg] = 15;
	robot.theta[robot.LeftShoulder] = -15;
	robot.theta[robot.RightShoulder] = 15;
	MoveTo();
}

//直立
void stand() {
	robot.theta[robot.RightUpperLeg] = 0;
	robot.theta[robot.LeftUpperLeg] = 0;
	robot.theta[robot.RightShoulder] = 0;
	robot.theta[robot.LeftShoulder] = 0;
	robot.theta[robot.RightLowerLeg] = 0;
	robot.theta[robot.LeftLowerLeg] = 0;
}

//站起来
void standup() {
	robot.theta[robot.RightUpperLeg] = 0;
	robot.theta[robot.LeftUpperLeg] = 0;
	robot.theta[robot.RightShoulder] = 0;
	robot.theta[robot.LeftShoulder] = 0;
	robot.theta[robot.RightLowerLeg] = 0;
	robot.theta[robot.LeftLowerLeg] = 0;
	robot.theta[robot.RightUpperArm] = 0;
	robot.theta[robot.LeftUpperArm] = 0;
	Move.x += 0.4;
	Move.y += 0.1;
}

void display()
{
	Shader skyboxShader("shaders/vshaderSky.glsl", "shaders/fshaderSky.glsl");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	painter->drawSky(skyboxShader, cameraSky);

	for (int i = 0; i < meshList.size() - 1; i++) {
		painter->drawMesh(meshList[i], painter->getOpenGLObj()[i], light, camera);
		painter->drawMesh(meshList[i], painter->getOpenGLObj()[i], light, camera, true);
	}

	if (isMoving) {
		if (Time % 50 == 0) {
			if (robot.theta[robot.RightUpperLeg] <= 0) {
				walk1();
			}
			else{
				walk2();
			}
		}
		Time++;
	}
	else if(Climb) {
		if (Time % 100 == 0) {
			if (robot.theta[robot.RightUpperLeg] >= 0) {
				climb1();
			}
			else{
				climb2();
			}
		}
		Time++;
	}
	else if (Down) {
		if (yDown == -8)
			Down = false;
		if (Time % 3 == 0) {
			xDown += 0.29;
			yDown -= 0.5;
		}
		Time++;
	}
	else if (Goodbye) {
		if (Time == 161) {
			downArm();
			Goodbye = false;
		}
		else if (Time == 0) {
			liftArm();
		}
		else if(Time % 20 == 0) {
			if (robot.theta[robot.LeftLowerArm] == 0 || 
				robot.theta[robot.LeftLowerArm] == -60) {
				goodbye1();
			}
			else {
				goodbye2();
			}
		}
		Time++;
	}
	else if(Elevator) {
		if (yElevator == 64)
			Elevator = false;
		yElevator += 0.5;
	}
	else if(Bridge) {
		if (bridge == 0)
			Bridge = false;
		if (Time % 3 == 0) {
			bridge -= 2;
		}
		Time++;
	}
	else if(Slide) {
		if (slide == 60)
			Slide = false;
		if (Time % 3 == 0) {
			slide -= 2;
		}
		Time++;
	}
	else {
		isMoving = false;
		Climb = false;
		Time = 0;
	}

	// ==============================================================
	// ============================ 场景 ============================
	// ==============================================================
	//地面
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(6.6, 0.1, 6.6));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, -60, 0));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	
	//底部
	ModelMatrix = cone->getModelMatrix();
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(9, 0.8, 9));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.09, -2.33, -0.13));
	ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GLfloat(180)), glm::vec3(1.0, 0.0, 0.0));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	
	//吊桥终点石柱
	ModelMatrix = cube->getModelMatrix();
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-10.0, 0, 0));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix, true);

	//电梯
	ModelMatrix = cube->getModelMatrix();
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1, 0.03, 1));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(2, -32.4 + yElevator, 0));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix, true);

	//吊桥板1
	ModelMatrix = glm::mat4(1.0);
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(1.6, 0.1, 4.0));
	ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GLfloat(bridge)), glm::vec3(0, 0, 1.0));
	glm::mat4 instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(-0.8, 0.03, 0.0));
	instance = glm::scale(instance, glm::vec3(0.84, 0.01, 0.4));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix * instance);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix * instance,true);

	//吊桥板2
	ModelMatrix = glm::mat4(1.0);
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-1.61, 0.1, 4.0));
	ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GLfloat(-bridge)), glm::vec3(0, 0, 1.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.84, 0.03, 0.0));
	instance = glm::scale(instance, glm::vec3(0.85, 0.01, 0.4));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix * instance);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix * instance,true);

	//滑梯
	ModelMatrix = glm::mat4(1.0);
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-0.55, 2.3, -2));
	ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GLfloat(-slide)), glm::vec3(0, 0, 1.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.84, 0.03, 0.0));
	instance = glm::scale(instance, glm::vec3(0.85, 0.01, 0.5));
	if (slide == 60) {
		instance = glm::translate(instance, glm::vec3(4.6, 0, 0));
		instance = glm::scale(instance, glm::vec3(5.6, 15, 1));
	}
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix* instance);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix* instance,true);

	//城堡1
	ModelMatrix = cube->getModelMatrix();
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1, 2, 1));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-12, 0.5, 0));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix, true);

	//城堡2
	ModelMatrix = cube->getModelMatrix();
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5, 0.7, 1.5));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-5, 1.5, -10));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix,true);
	
	//柱子1
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1, 0.5, 0.1));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-9, 3, 9));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix,true);
	//柱子2
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, 0, -18));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix,true);
	//柱子3
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(18, 0, 0));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix,true);
	//柱子4
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, 0, 18));
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cube, painter->getOpenGLObj()[0], light, camera, ModelMatrix,true);

	//屋顶2
	ModelMatrix = cone->getModelMatrix();
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.4, 0.7, 1.4));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.37, -0.38 ,-1.22));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix,true);
	//底部2
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.6, 0.35, 0.6));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, -3.87, 0));
	ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GLfloat(180)), glm::vec3(1.0, 0.0, 0.0));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix,true);

	//=========台阶=========//
	//台阶1
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.4, 1.3, 0.4));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-0.7, -0.69 - firstStair - up, -4.9 + 0.57));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix,true);
	//台阶2
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.1, -up, 0.57));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix, true);
	//台阶3
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.1, -up, 0.57));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix, true);
	//台阶4
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.1, -up, 0.57));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix, true);
	//台阶5
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.1, -up, 0.57));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix, true);
	//台阶6
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.1, -up, 0.57));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix, true);
	//台阶7
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.1, -up, 0.57));
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix);
	painter->drawDynamicMesh(cone, painter->getOpenGLObj()[1], light, camera, ModelMatrix, true);

	//顶面1
	ModelMatrix = disk->getModelMatrix();
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, 6.2 * firstStair, 0));
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix);
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix, true);
	//顶面2
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.115, 2.55 * up, -0.67));
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix);
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix, true);
	//顶面3
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.115, 2.55 * up, -0.67));
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix);
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix, true);
	//顶面4
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.12, 2.55 * up, -0.67));
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix);
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix, true);
	//顶面5
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.12, 2.55 * up, -0.67));
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix);
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix, true);
	//顶面6
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.12, 2.55 * up, -0.67));
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix);
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix, true);
	//顶面7
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.12, 2.55 * up, -0.67));
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix);
	painter->drawDynamicMesh(disk, painter->getOpenGLObj()[2], light, camera, ModelMatrix, true);
	

	// ==============================================================
	// ============================ 小人 ============================
	// ==============================================================
	// 物体的变换矩阵
	glm::mat4 modelMatrix = glm::mat4(1.0);

	// 保持变换矩阵的栈
	MatrixStack mstack;

	// 躯干
	modelMatrix = glm::translate(modelMatrix, glm::vec3(3.55, -5.2 + yElevator/11, 4.0));	//初始位置和坐电梯的移动
	modelMatrix = glm::translate(modelMatrix, glm::vec3(xDown, yDown, 0.0));	//滑滑梯时的移动
	modelMatrix = glm::translate(modelMatrix, Move);		//平时的移动
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.Torso]), glm::vec3(0.0, 1.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, 0.1 * robot.TORSO_HEIGHT, 0.0));
	painter->drawDynamicMesh(Torso, TorsoObject, light, camera, modelMatrix* instance);

	// =========== 头部 ===========
	mstack.push(modelMatrix); // 保存躯干变换矩阵
	// 头部
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0, robot.TORSO_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.Head]), glm::vec3(0.0, 1.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, 0.5 * robot.HEAD_HEIGHT, 0.0));
	instance = glm::scale(instance, glm::vec3(robot.head, robot.head, robot.head));
	painter->drawDynamicMesh(Head, HeadObject, light, camera, modelMatrix * instance);

	// 帽子
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, 1.39 * robot.HEAD_HEIGHT, -0.44 * robot.HEAD_HEIGHT));
	instance = glm::rotate(instance, glm::radians(GLfloat(-25)), glm::vec3(1.0, 0.0, 0.0));
	painter->drawDynamicMesh(Hat, HatObject, light, camera, modelMatrix* instance);
	
	modelMatrix = mstack.pop(); // 恢复躯干变换矩阵


	// =========== 左臂 ===========
	mstack.push(modelMatrix);
	// 左肩膀
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5 * robot.TORSO_WIDTH - robot.ARM_WIDTH, robot.TORSO_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftShoulder]), glm::vec3(1.0, 0.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(robot.TORSO_WIDTH * 0.39, -1.8 * robot.ARM_HEIGHT, 0.0));
	instance = glm::scale(instance, glm::vec3(robot.shoulder, robot.shoulder, robot.shoulder));
	painter->drawDynamicMesh(Shoulder, ShoulderObject, light, camera, modelMatrix* instance);

	//左大臂
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.TORSO_WIDTH - 1.1 * robot.ARM_WIDTH, -0.48 * robot.TORSO_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftUpperArm] - 20), glm::vec3(0.0, 0.0, 1.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(robot.TORSO_WIDTH * 0.03, -1.2 * robot.ARM_HEIGHT, 0.0));
	painter->drawDynamicMesh(Arm, ArmObject, light, camera, modelMatrix* instance);

	//左小臂
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -1.857 * robot.ARM_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftLowerArm]), glm::vec3(0.0, 0.0, 1.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(robot.TORSO_WIDTH * 0.03, -0.857 * robot.ARM_HEIGHT, 0.0));
	painter->drawDynamicMesh(Arm, ArmObject, light, camera, modelMatrix* instance);

	modelMatrix = mstack.pop();

	// =========== 右臂 ===========
	mstack.push(modelMatrix);
	// 右肩膀
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5 * robot.TORSO_WIDTH + robot.ARM_WIDTH, robot.TORSO_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightShoulder]), glm::vec3(1.0, 0.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(-robot.TORSO_WIDTH * 0.39, -1.8 * robot.ARM_HEIGHT, 0.0));
	instance = glm::scale(instance, glm::vec3(robot.shoulder, robot.shoulder, robot.shoulder));
	painter->drawDynamicMesh(Shoulder, ShoulderObject, light, camera, modelMatrix* instance);

	// 右大臂
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5 * robot.TORSO_WIDTH + 1.1 * robot.ARM_WIDTH, -0.48 * robot.TORSO_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightUpperArm] + 20), glm::vec3(0.0, 0.0, 1.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(-0.03 * robot.TORSO_WIDTH, -1.2 * robot.ARM_HEIGHT, 0.0));
	painter->drawDynamicMesh(Arm, ArmObject, light, camera, modelMatrix* instance);

	//右小臂
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -1.857 * robot.ARM_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightLowerArm]), glm::vec3(0.0, 0.0, 1.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(-0.03 * robot.TORSO_WIDTH, -0.857 * robot.ARM_HEIGHT, 0.0));
	painter->drawDynamicMesh(Arm, ArmObject, light, camera, modelMatrix* instance);

	modelMatrix = mstack.pop();

	// =========== 左腿 ===========
	mstack.push(modelMatrix);
	//左大腿
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.2 * robot.LEG_WIDTH, 0.0, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftUpperLeg]), glm::vec3(1.0, 0.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -3.5 * robot.LEG_HEIGHT, 0.0));
	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	painter->drawDynamicMesh(Leg, LegObject, light, camera, modelMatrix* instance);

	//左小腿
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -4.2 * robot.LEG_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.LeftLowerLeg]), glm::vec3(1.0, 0.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -1.1 * robot.LEG_HEIGHT, 0.0));
	// 乘以来自父物体的模型变换矩阵，绘制当前物体
	painter->drawDynamicMesh(Leg, LegObject, light, camera, modelMatrix* instance);

	modelMatrix = mstack.pop();


	// =========== 右腿 ===========
	mstack.push(modelMatrix);
	//右大腿
	modelMatrix = glm::translate(modelMatrix, glm::vec3(1.2 * robot.LEG_WIDTH, 0.0, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightUpperLeg]), glm::vec3(1.0, 0.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -3.5 * robot.LEG_HEIGHT, 0.0));
	painter->drawDynamicMesh(Leg, LegObject, light, camera, modelMatrix* instance);

	//右小腿
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -4.2 * robot.LEG_HEIGHT, 0.0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(robot.theta[robot.RightLowerLeg]), glm::vec3(1.0, 0.0, 0.0));
	instance = glm::mat4(1.0);
	instance = glm::translate(instance, glm::vec3(0.0, -1.1 * robot.LEG_HEIGHT, 0.0));
	painter->drawDynamicMesh(Leg, LegObject, light, camera, modelMatrix* instance);
	modelMatrix = mstack.pop();
	
}


void printHelp()
{	
	std::cout << "Keyboard Usage" << std::endl;
	std::cout <<
		"[Person]" << std::endl <<
		"0:		start walking" << std::endl <<
		"1:		stand" << std::endl <<
		"2:		end climbing" << std::endl <<
		"3:		stand up" << std::endl <<
		"c:		start climbing" << std::endl <<
		"LEFT:		turn left" << std::endl <<
		"RIGHT:		turn right" << std::endl <<
		"UP:		step up" << std::endl <<
		"DOWN:		sit down" << std::endl <<

		std::endl <<
		"[Device]" << std::endl <<
		"e/E:		elevator rise" << std::endl <<
		"b/B:		generate bridge" << std::endl <<
		"p/P:		stairs rise" << std::endl <<
		"s/S:		generate slide" << std::endl <<
		
		std::endl <<
		"[Light]" << std::endl <<
		"x/X:		Decrease/Increase x" << std::endl <<
		"y/Y:		Decrease/Increase y" << std::endl <<
		"z/Z:		Decrease/Increase z" << std::endl << 

		std::endl <<
		"[Camera]" << std::endl <<		
		"u/U:		Increase/Decrease the rotate angle" << std::endl <<
		"i/I:		Increase/Decrease the up angle" << std::endl <<
		"o/O:		Increase/Decrease the camera radius" << std::endl << std::endl;

}

// 键盘响应函数
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	float tmp;
	glm::vec4 ambient;
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {

		if (key == GLFW_KEY_E) {
			Elevator = true;
		}
		else if (key == GLFW_KEY_B) {
			Bridge = true;
		}
		else if (key == GLFW_KEY_P) {
			up = 0.1;
			firstStair = 0.07;
		}
		else if (key == GLFW_KEY_S) {
			Slide = true;
		}
		else if (key == GLFW_KEY_0) {
			isMoving = true;
		}
		else if (key == GLFW_KEY_1) {
			isMoving = false;
			Climb = false;
			stand();
		}
		else if (key == GLFW_KEY_2) {
			isMoving = false;
			Climb = false;
			stand();
			Move.x += 0.12;
			Move.y += 0.09;
			robot.theta[robot.Torso] += 15;
		}
		else if (key == GLFW_KEY_3) {
			standup();
		}
		else if (key == GLFW_KEY_C) {
			Climb = true;
			robot.theta[robot.Torso] -= 15;
		}
		else if (key == GLFW_KEY_D) {
			Down = true;
		}
		else if (key == GLFW_KEY_G) {
			Goodbye = true;
		}
		else if (key == GLFW_KEY_UP) {
			climb();
		}
		else if (key == GLFW_KEY_DOWN) {
			sit();
		}
		else if (key == GLFW_KEY_LEFT) {
			if (direction != 1)
				direction--;
			else
				direction = 4;
			Selected_mesh = robot.Torso;
			robot.theta[Selected_mesh] += 90.0;
			if (robot.theta[Selected_mesh] > 360.0)
				robot.theta[Selected_mesh] -= 360.0;
		}
		else if (key == GLFW_KEY_RIGHT) {
			if (direction != 4)
				direction++;
			else
				direction = 1;
			Selected_mesh = robot.Torso;
			robot.theta[Selected_mesh] -= 90.0;
			if (robot.theta[Selected_mesh] > 360.0)
				robot.theta[Selected_mesh] -= 360.0;
		}
		else if (key == GLFW_KEY_X && mode == 0x0000) {
			glm::vec3 pos = light->getTranslation();
			pos.x -= 2;
			light->setTranslation(pos);
		}
		else if (key == GLFW_KEY_X && mode == GLFW_MOD_SHIFT) {
			glm::vec3 pos = light->getTranslation();
			pos.x += 2;
			light->setTranslation(pos);
		}
		else if (key == GLFW_KEY_Y && mode == 0x0000) {
			glm::vec3 pos = light->getTranslation();
			pos.y -= 2;
			light->setTranslation(pos);
		}
		else if (key == GLFW_KEY_Y && mode == GLFW_MOD_SHIFT) {
			glm::vec3 pos = light->getTranslation();
			pos.y += 2;
			light->setTranslation(pos);
		}
		else if (key == GLFW_KEY_Z && mode == 0x0000) {
			glm::vec3 pos = light->getTranslation();
			pos.z -= 2;
			light->setTranslation(pos);
		}
		else if (key == GLFW_KEY_Z && mode == GLFW_MOD_SHIFT) {
			glm::vec3 pos = light->getTranslation();
			pos.z += 2;
			light->setTranslation(pos);
		}
		else if (key == GLFW_KEY_I && mode == 0x0000) {
			camera->upAngle += 5.0;
			if (camera->upAngle >= 180)
				camera->upAngle = camera->upAngle - 360;

			cameraSky->upAngle -= 5.0;
			if (cameraSky->upAngle <= -180)
				cameraSky->upAngle = cameraSky->upAngle + 360;
		}
		else if (key == GLFW_KEY_I && mode == GLFW_MOD_SHIFT) {
			camera->upAngle -= 5.0;
			if (camera->upAngle <= -180)
				camera->upAngle = camera->upAngle + 360;

			cameraSky->upAngle += 5.0;
			if (cameraSky->upAngle >= 180)
				cameraSky->upAngle = cameraSky->upAngle - 360;
		}
		if (key == GLFW_KEY_U && mode == GLFW_MOD_SHIFT) {
			camera->rotateAngle += 5.0;
			if (camera->rotateAngle > 180)
				camera->rotateAngle = camera->rotateAngle - 360;

			cameraSky->rotateAngle -= 5.0;
			if (cameraSky->rotateAngle < -180)
				cameraSky->rotateAngle = cameraSky->rotateAngle + 360;
		}
		else if (key == GLFW_KEY_U && mode == 0x0000) {
			camera->rotateAngle -= 5.0;
			if (camera->rotateAngle < -180)
				camera->rotateAngle = camera->rotateAngle + 360;

			cameraSky->rotateAngle += 5.0;
			if (cameraSky->rotateAngle > 180)
				cameraSky->rotateAngle = cameraSky->rotateAngle - 360;
		}
		else {
			camera->keyboard(key, action, mode);
			cameraSky->keyboard(key, action, mode);
		}
	}
}

// 重新设置窗口
void reshape(GLsizei w, GLsizei h)
{
	glViewport(0, 0, w, h);
}

void cleanData() {
	// 释放内存

	delete camera;
	camera = NULL;

	delete light;
	light = NULL;

	painter->cleanMeshes();

	delete painter;
	painter = NULL;

	for (int i = 0; i < meshList.size(); i++) {
		delete meshList[i];
	}
	meshList.clear();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main(int argc, char** argv)
{
	// 初始化GLFW库，必须是应用程序调用的第一个GLFW函数
	glfwInit();

	// 配置GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// 配置窗口属性
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "2019152056_韩沂桐_期末大作业_", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// 调用任何OpenGL的函数之前初始化GLAD
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Init mesh, shaders, buffer
	init();
	// 输出帮助信息
	printHelp();
	// 启用深度测试
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		display();
		// 交换颜色缓冲 以及 检查有没有触发什么事件（比如键盘输入、鼠标移动等）
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	cleanData();
	return 0;
}

// 每当窗口改变大小，GLFW会调用这个函数并填充相应的参数供你处理。
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}
