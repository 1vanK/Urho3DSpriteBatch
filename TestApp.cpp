#include <Urho3D/Urho3DAll.h>
#include "SpriteBatch.h"

#define CACHE GetSubsystem<ResourceCache>()
#define RENDERER GetSubsystem<Renderer>()
#define INPUT GetSubsystem<Input>()
#define DEBUG_HUD GetSubsystem<DebugHud>()
#define FILE_SYSTEM GetSubsystem<FileSystem>()
#define UI GetSubsystem<UI>()
#define UI_ROOT UI->GetRoot()
#define GRAPHICS GetSubsystem<Graphics>()
#define GLOBAL GetSubsystem<Global>()

#define GET_MATERIAL CACHE->GetResource<Material>
#define GET_MODEL CACHE->GetResource<Model>

class Game : public Application
{
    URHO3D_OBJECT(Game, Application);

public:
    SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;

    SpriteBatch* spriteBatch_;
    float fpsTimeCounter_ = 0.0f;
    int fpsFrameCounter_ = 0;
    int fpsValue_ = 0;

    Game(Context* context) : Application(context)
    {
    }

    void Setup()
    {
        engineParameters_["WindowTitle"] = GetTypeName();
        //engineParameters_["LogName"] = FILE_SYSTEM->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
        engineParameters_["FullScreen"] = false;
        engineParameters_["WindowWidth"] = 800;
        engineParameters_["WindowHeight"] = 600;
        //engineParameters_["ResourcePaths"] = "GameData;Data;CoreData";
    }

    void Start()
    {
        CreateScene();
        SetupViewport();
        SubscribeToEvents();

        XMLFile* xmlFile = CACHE->GetResource<XMLFile>("UI/DefaultStyle.xml");
        DebugHud* debugHud = engine_->CreateDebugHud();
        debugHud->SetDefaultStyle(xmlFile);

        spriteBatch_ = new SpriteBatch(context_);
    }

    void SetupViewport()
    {
        SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
        RENDERER->SetViewport(0, viewport);
    }

    void CreateScene()
    {
        scene_ = new Scene(context_);
        scene_->CreateComponent<Octree>();

        /*Node* planeNode = scene_->CreateChild("Plane");
        planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
        StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
        planeObject->SetModel(CACHE->GetResource<Model>("Models/Plane.mdl"));
        planeObject->SetMaterial(CACHE->GetResource<Material>("Materials/StoneTiled.xml"));

        Node* lightNode = scene_->CreateChild("DirectionalLight");
        lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
        Light* light = lightNode->CreateComponent<Light>();
        light->SetColor(Color(0.5f, 0.5f, 0.5f));
        light->SetLightType(LIGHT_DIRECTIONAL);
        light->SetCastShadows(true);
        light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
        light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
        //light->SetShadowIntensity(0.5f);

        Node* zoneNode = scene_->CreateChild("Zone");
        Zone* zone = zoneNode->CreateComponent<Zone>();
        zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
        zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
        zone->SetFogColor(Color(0.4f, 0.5f, 0.8f));
        zone->SetFogStart(100.0f);
        zone->SetFogEnd(300.0f);

        const unsigned NUM_OBJECTS = 200;
        for (unsigned i = 0; i < NUM_OBJECTS; ++i)
        {
            Node* mushroomNode = scene_->CreateChild("Mushroom");
            mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
            mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
            mushroomNode->SetScale(0.5f + Random(2.0f));
            StaticModel* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
            mushroomObject->SetModel(CACHE->GetResource<Model>("Models/Mushroom.mdl"));
            mushroomObject->SetMaterial(CACHE->GetResource<Material>("Materials/Mushroom.xml"));
            mushroomObject->SetCastShadows(true);
        }*/

        cameraNode_ = scene_->CreateChild("Camera");
        cameraNode_->CreateComponent<Camera>();
        cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
    }

    void MoveCamera(float timeStep)
    {
        const float MOVE_SPEED = 20.0f;
        const float MOUSE_SENSITIVITY = 0.1f;

        IntVector2 mouseMove = INPUT->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

        if (INPUT->GetKeyDown('W'))
            cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
        if (INPUT->GetKeyDown('S'))
            cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
        if (INPUT->GetKeyDown('A'))
            cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
        if (INPUT->GetKeyDown('D'))
            cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

        if (INPUT->GetKeyPress(KEY_F2))
            DEBUG_HUD->ToggleAll();
    }

    void SubscribeToEvents()
    {
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Game, HandleUpdate));
        SubscribeToEvent(E_ENDVIEWRENDER, URHO3D_HANDLER(Game, HandleEndViewRender));
    }

    void HandleUpdate(StringHash eventType, VariantMap& eventData)
    {
        using namespace Update;

        float timeStep = eventData[P_TIMESTEP].GetFloat();

        MoveCamera(timeStep);

        fpsTimeCounter_ += timeStep;
        fpsFrameCounter_++;

        if (fpsTimeCounter_ >= 1.0f)
        {
            fpsTimeCounter_ = 0.0f;
            fpsValue_ = fpsFrameCounter_;
            fpsFrameCounter_ = 0;
        }
    }

    void HandleEndViewRender(StringHash eventType, VariantMap& eventData)
    {
        Texture2D* texture = GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Urho2D/Ball.png");

        spriteBatch_->Begin();

        for (int i = 0; i < 20000; i++)
            spriteBatch_->Draw(texture, Vector2(Random(0.0f, 800.0f), Random(0.0f, 600.0f)), nullptr, Color::WHITE, Vector2(0, 0));

        spriteBatch_->DrawString(String("FPS: ") + String(fpsValue_), Vector2(50.0f, 50.0f),
            CACHE->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 40, Color::RED);

        spriteBatch_->End();
    }
};

URHO3D_DEFINE_APPLICATION_MAIN(Game)
