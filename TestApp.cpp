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
        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_WINDOW_WIDTH] = 800;
        engineParameters_[EP_WINDOW_HEIGHT] = 600;
        engineParameters_[EP_FRAME_LIMITER] = false;
    }

    void Start()
    {
        CreateScene();
        SetupViewport();
        SubscribeToEvents();

        XMLFile* xmlFile = CACHE->GetResource<XMLFile>("UI/DefaultStyle.xml");
        DebugHud* debugHud = engine_->CreateDebugHud();
        debugHud->SetDefaultStyle(xmlFile);

        spriteBatch_ = new SpriteBatch(context_, 600);
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

    float angle_ = 0.0f;
    float scale_ = 0.0f;

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

        angle_ += timeStep * 100.0f;
        angle_ = fmod(angle_, 360.0f);

        scale_ += timeStep;
    }

    void HandleEndViewRender(StringHash eventType, VariantMap& eventData)
    {
        Texture2D* ball = GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Urho2D/Ball.png");
        Texture2D* head = GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Urho2D/imp/imp_head.png");

        // Можно так очищать, а можно цвет зоны задать.
        //GetSubsystem<Graphics>()->Clear(CLEAR_COLOR, Color::GREEN);

        spriteBatch_->Begin();

        for (int i = 0; i < 20000; i++)
            spriteBatch_->Draw(ball, Vector2(Random(0.0f, 800.0f), Random(0.0f, 600.0f)), nullptr, Color::WHITE);

        spriteBatch_->Draw(head, Vector2(200.0f, 200.0f), nullptr, Color::WHITE, 0.0f, Vector2::ZERO, Vector2::ONE, SBE_FLIP_BOTH);

        float scale = cos(scale_) + 1.0f; // cos возвращает значения в диапазоне [-1, 1], значит scale будет в диапазоне [0, 2].
        Vector2 origin = Vector2(head->GetWidth() * 0.5f, head->GetHeight() * 0.5f);
        spriteBatch_->Draw(head, Vector2(400.0f, 300.0f), nullptr, Color::WHITE, angle_, origin, Vector2(scale, scale));

        spriteBatch_->DrawString(String("FPS: ") + String(fpsValue_),
            CACHE->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 40.0f, Vector2(50.0f, 50.0f), Color::RED);

        spriteBatch_->DrawString("Mirrored Text", CACHE->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 40.0f,
            Vector2(250.0f, 200.0f), Color::RED, 0.0f, Vector2::ZERO, Vector2::ONE, SBE_FLIP_BOTH);

        spriteBatch_->DrawString("Some Text", CACHE->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 40.0f,
            Vector2(400.0f, 300.0f), Color::BLUE, angle_, Vector2::ZERO, Vector2(scale, scale));

        spriteBatch_->End();
    }
};

URHO3D_DEFINE_APPLICATION_MAIN(Game)
