#pragma once

#include <bullet/btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <utils/physics.h>

struct WheelInfo {
    float gVehicleSteering = 0.f;
    float steeringIncrement = 0.08f;
    float steeringClamp = 0.5f;
    // friction of the single wheel
    float friction = 10000;  //BT_LARGE_FLOAT;
    float suspensionStiffness = 5.f;//20.f;
    float suspensionDamping = 2.3f;
    float suspensionCompression = 4.4f;
    float rollInfluence = 0.1f;  //1.0f;
    float suspensionRestLength = 1.320f;

    float radius = 0.4f;
    float width = 0.3f;
};

class Vehicle {
public:
    Vehicle(const glm::vec3 &chassisBoxSize, const WheelInfo &wheelInfo): WheelInfo(wheelInfo), vehicle(vehicle) {
        btTransform tr;
        btVector3 chassisBox(chassisBoxSize.x, chassisBoxSize.y, chassisBoxSize.z);
        Physics &bulletSimulation = Physics::getInstance();
        btCollisionShape *chassisShape = new btBoxShape(chassisBox);
        Chassis = bulletSimulation.localCreateRigidBody(800, tr, chassisShape);
        auto vehicleRayCaster = new btDefaultVehicleRaycaster(bulletSimulation.dynamicsWorld);
        btRaycastVehicle::btVehicleTuning tuning;
        vehicle = btRaycastVehicle(tuning, Chassis, vehicleRayCaster);
        //never deactivate the vehicle
        Chassis->setActivationState(DISABLE_DEACTIVATION);

        bulletSimulation.dynamicsWorld->addVehicle(&vehicle);
        
        float connectionHeight = wheelInfo.radius;

        int rightIndex = 0, upIndex = 1, forwardIndex = 2;
        //choose coordinate system
        vehicle.setCoordinateSystem(rightIndex, upIndex, forwardIndex);

        btVector3 wheelDirectionCS0(0, -1, 0);
        btVector3 wheelAxleCS(-1, 0, 0);

        btVector3 connectionPointCS0(chassisBoxSize.x - (0.3 * wheelInfo.width), connectionHeight, chassisBoxSize.z - wheelInfo.radius);
        vehicle.addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, wheelInfo.suspensionRestLength, wheelInfo.radius, tuning, true);

        connectionPointCS0 = btVector3(-chassisBoxSize.x + (0.3 * wheelInfo.width), connectionHeight, chassisBoxSize.z - wheelInfo.radius);
        vehicle.addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, wheelInfo.suspensionRestLength, wheelInfo.radius, tuning, true);

        connectionPointCS0 = btVector3(-chassisBoxSize.x + (0.3 * wheelInfo.width), connectionHeight, -chassisBoxSize.z + wheelInfo.radius);
        vehicle.addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, wheelInfo.suspensionRestLength, wheelInfo.radius, tuning, false);

        connectionPointCS0 = btVector3(chassisBoxSize.x - (0.3 * wheelInfo.width), connectionHeight, -chassisBoxSize.z + wheelInfo.radius);
        vehicle.addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, wheelInfo.suspensionRestLength, wheelInfo.radius, tuning, false);
    
        updateWheelProperty();

        // reset chassis
        Chassis->setCenterOfMassTransform(btTransform::getIdentity());
        Chassis->setLinearVelocity(btVector3(0, 0, 0));
        Chassis->setAngularVelocity(btVector3(0, 0, 0));
        bulletSimulation.dynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(Chassis->getBroadphaseHandle(), bulletSimulation.dynamicsWorld->getDispatcher());
        vehicle.resetSuspension();

        updateWheelTransform();
    }

    btRigidBody *Chassis;
    WheelInfo WheelInfo;
    // time to shoot again
    float ShootCooldown = 1.f; // in seconds
    float EngineForce = 0.f;
    float BreakingForce = 0.f;
    float Steering = 0.f;

    // HACK: kinda hack, we do this to use the transformation of the chassis and the wheel... we should return them?
    btRaycastVehicle &GetBulletVehicle() {
        return vehicle;
    }

    float GetSpeed() {
        return vehicle.getCurrentSpeedKmHour();
    }

    void Accelerate() {
        EngineForce = maxEngineForce; 
    }
    
    void Decelerate() {
        EngineForce = -maxEngineForce; 
    }

    void SteerLeft() {
        Steering += steeringIncrement;
        isSteering = true;
    }

    void SteerRight() {
        Steering -= steeringIncrement;
        isSteering = true;
    }

    void Shoot() {
        if(shootTimer > 0.f) return;
        // TODO: HACK: shared with main (draw) if we want to keep this we should refactor this in a separate class
        glm::vec3 sphereSize = glm::vec3(0.2f, 0.2f, 0.2f);

        // reset the timer
        shootTimer = ShootCooldown;

        Physics &bulletSimulation = Physics::getInstance();

        // we need a initial rotation, even if useless for a sphere
        glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f);
        // initial velocity of the bullet
        float shootInitialSpeed = 30.0f;
        btVector3 shootDirection(0.f, 0.f, 5.f);
        // position of the vehicle
        auto transform = vehicle.getChassisWorldTransform(); 
        // spawn position of the bullet
        auto spherePosition = transform * shootDirection;
        // TODO: use toGLM function from main
        glm::vec3 position(spherePosition.getX(), spherePosition.getY(), spherePosition.getZ());
        // rigid body of the bullet, we create a Rigid Body with mass = 1
        btRigidBody *sphere = bulletSimulation.createRigidBody(SPHERE, position, sphereSize, rot, 1.0f, 0.3f, 0.3f);

        // we apply the impulse and shoot the bullet in the scene
        // N.B.) the graphical aspect of the bullet is treated in the rendering loop
        // for the impulse keep the same direction but remove any translation from the matrix
        btVector3 impulse = (spherePosition - transform.getOrigin()).normalize() * shootInitialSpeed;
        sphere->applyCentralImpulse(impulse);
    }

    void ResetRotation() {
        // sometime the car turn around so we ripristinate the spawn orientation to allow the player drive again
        btTransform chassisTransform = vehicle.getChassisWorldTransform();
        auto origin = chassisTransform.getOrigin();
        chassisTransform.setIdentity();
        chassisTransform.setOrigin(origin);
        Chassis->clearForces();
        Chassis->setWorldTransform(chassisTransform);
        Chassis->getMotionState()->setWorldTransform(chassisTransform);
    }

    void Update(float deltaTime) {
        // decrease bullet cooldown
        shootTimer -= deltaTime;
        
        // lineary resetting the steering angle to 0 until ge straight again
        float newAngle = Steering;
        if(!isSteering) {
            if(fabs(Steering) > steeringIncrement) {
                newAngle = Steering * (1 - .01);
            } else {
                newAngle = 0;
            }
        }
        // clamp steering to avoid out of bound turning
        Steering = __min(__max(newAngle, -steeringClamp), steeringClamp);

        // vehicle physics update
        int wheelIndex = 2;
        vehicle.applyEngineForce(EngineForce, wheelIndex);
        vehicle.setBrake(BreakingForce, wheelIndex);
        wheelIndex = 3;
        vehicle.applyEngineForce(EngineForce, wheelIndex);
        vehicle.setBrake(BreakingForce, wheelIndex);

        wheelIndex = 0;
        vehicle.setSteeringValue(Steering, wheelIndex);
        wheelIndex = 1;
        vehicle.setSteeringValue(Steering, wheelIndex);
        
        // vehicle property update
        updateWheelProperty();
        // reset property for update
        EngineForce = 0.f;
        BreakingForce = 0.f;
        isSteering = false;
    }

    // TODO: for imgui tweeking
    float maxEngineForce = 2000.f;
private:
    float shootTimer = 0.f;
    const float steeringIncrement = 0.02f;
    const float steeringClamp = 0.5f;
    bool isSteering;
    btRaycastVehicle vehicle;

    void updateWheelProperty() {
        for (int i = 0; i < vehicle.getNumWheels(); i++) {
            btWheelInfo &wheel = vehicle.getWheelInfo(i);
            wheel.m_suspensionStiffness = WheelInfo.suspensionStiffness;
            wheel.m_wheelsDampingRelaxation = WheelInfo.suspensionDamping;
            wheel.m_wheelsDampingCompression = WheelInfo.suspensionCompression;
            wheel.m_frictionSlip = WheelInfo.friction;
            wheel.m_rollInfluence = WheelInfo.rollInfluence;
            wheel.m_suspensionRestLength1 = WheelInfo.suspensionRestLength;
        }
    }

    void updateWheelTransform() {
        for (int i = 0; i < vehicle.getNumWheels(); i++) {
            //synchronize the wheels with the (interpolated) chassis worldtransform
            vehicle.updateWheelTransform(i, true);
        }
    }
};