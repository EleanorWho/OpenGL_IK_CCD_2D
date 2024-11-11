#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "IKbone.h"

#include "stb_image.h"
#include <iostream>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 1.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

DirLight BasicLight;

glm::vec2 targetPos = glm::vec2(0.0f);

// initialize IKbone
IKClass ikSolver;
glm::vec3 rootPos(0.0f, 0.0f, 0.0f);
glm::vec3 jointPos(0.5f, 0.0f, 0.0f);
glm::vec3 joint2Pos(1.0f, 0.0f, 0.0f);		// if you need to add more bones, just define a new position here

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Real-time Animation Assignment 2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader modelShader("vertexShaders/IK_vs.txt", "fragmentShaders/IK_fs.txt");

    // load models
    // -----------
    Model boneModel("bone/newBone.obj");

    ikSolver.chain.addJoint(IKJoint(rootPos));
    ikSolver.chain.addJoint(IKJoint(jointPos));
    //ikSolver.chain.addJoint(IKJoint(joint2Pos));		// if you need to add a new bone, just add the new bone to the chain

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // direct light settings
        BasicLight.direction = glm::vec3(16.0f, -10.0f, -7.0f);
        BasicLight.color = glm::vec3(1.0f);
        BasicLight.ambient = glm::vec3(0.3f);
        BasicLight.diffuse = glm::vec3(0.5f);
        BasicLight.specular = glm::vec3(0.2f);

        // update bone information
        // -----------------------
        ikSolver.setTarget(glm::vec3(targetPos, 0.0f));
		
        // apply CCD algorithm
        ikSolver.applyCCD();
        glm::mat4 rootTransform = ikSolver.getRootTransform();

        // check the joint position after update
        for (auto& joint : ikSolver.chain.joints) {
            std::cout << "Joint Position: x = " << (joint.position).x << ", y = " << (joint.position).y << std::endl;
        }

        // update position
        // ---------------
        std::cout << "Now the target position is: x = " << targetPos.x << ", y = " <<targetPos.y << std::endl;
        
        // draw the model
        modelShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        modelShader.setVec3("diffuse_color", glm::vec3(1.0f, 1.0f, 0.8f));
        modelShader.setVec3("specular_color", glm::vec3(1.0f));
        modelShader.setVec3("dirLight.color", BasicLight.color);
        modelShader.setVec3("dirLight.direction", BasicLight.direction);
        modelShader.setVec3("dirLight.ambient", BasicLight.ambient);
        modelShader.setVec3("dirLight.diffuse", BasicLight.diffuse);
        modelShader.setVec3("dirLight.specular", BasicLight.specular);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        for (const auto& joint : ikSolver.chain.joints) {
            modelMatrix = glm::mat4(1.0f);

            // translate the modelMatrix to the global position of the joints
            modelMatrix = glm::translate(modelMatrix, joint.position);

            // joint global rotation
            modelMatrix *= glm::toMat4(joint.globalRotation);

            modelShader.setMat4("model", modelMatrix);
            boneModel.Draw(modelShader);
        }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    // check if the left button of the mouse is pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        // Convert screen position to normalized device coordinates
        float xNDC = (2.0f * xposIn) / SCR_WIDTH - 1.0f;
        float yNDC = 1.0f - (2.0f * yposIn) / SCR_HEIGHT;

        glm::vec4 clipCoords = glm::vec4(xNDC, yNDC, -1.0f, 1.0f);

        // Convert clip coordinates to eye coordinates
        glm::vec4 eyeCoords = glm::inverse(projection) * clipCoords;
        eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);

        // Convert eye coordinates to world coordinates
        glm::vec4 worldCoords = glm::inverse(view) * eyeCoords;

        glm::vec3 rayWorld = glm::normalize(glm::vec3(worldCoords));

        float planeZ = 0.0f;

        float t = (planeZ - camera.Position.z) / rayWorld.z;
        glm::vec3 targetPosOnPlane = camera.Position + rayWorld * t;

        // Use the worldPos for target position in IK calculations
        targetPos = glm::vec2(targetPosOnPlane.x, targetPosOnPlane.y);
    }
    else
    {
        firstMouse = true; // Reset the initial state if the mouse button is not pressed
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

