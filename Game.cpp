#include "Urho3DAll.h"
#include "Game.h"


DEFINE_APPLICATION_MAIN(Game)


Node* pickedNode = nullptr;
Vector3 deltaPickedNode = Vector3::ZERO;

Node* invisPlaneNode = nullptr;

Node* currentGrenade = nullptr;

Vector3 startPoint;

Node* rubberLeft = nullptr;
Node* rubberRight = nullptr;



Game::Game(Context* context) :
    Application(context),
    yaw_(0.0f),
    pitch_(0.0f)
{
}


void Game::Setup()
{
	engineParameters_["WindowTitle"] = GetTypeName();
	//engineParameters_["LogName"] = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
	engineParameters_["FullScreen"] = false;
	engineParameters_["Headless"] = false;
	engineParameters_["WindowWidth"] = 800;
	engineParameters_["WindowHeight"] = 600;
	engineParameters_["ResourcePaths"] = "Data;CoreData;MyData";
}


void Game::Start()
{
	GetSubsystem<Input>()->SetMouseVisible(true);

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    SharedPtr<File> file = cache->GetFile("Levels/00001.xml");
    scene_ = new Scene(context_);
    scene_->LoadXML(*file);
	
    Renderer* renderer = GetSubsystem<Renderer>();
    Node* cameraNode = scene_->GetChild("Camera");
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    Node* slingshotNode = scene_->GetChild("Slingshot");
    startPoint = slingshotNode->GetPosition() + Vector3(0.0f, 2.0f, 0.0f);
    
    SubscribeToEvents();
	
	XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	DebugHud* debugHud = engine_->CreateDebugHud();
	debugHud->SetDefaultStyle(xmlFile);
}


void Game::SetupViewport()
{
}


void Game::CreateScene()
{
}


void Game::MoveCamera(float timeStep)
{
	Input* input = GetSubsystem<Input>();

	const float MOVE_SPEED = 20.0f;
	const float MOUSE_SENSITIVITY = 0.1f;

	IntVector2 mouseMove = input->GetMouseMove();
	yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
	pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
	pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    Node* cameraNode = scene_->GetChild("Camera");

    cameraNode->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

	if (input->GetKeyDown('W'))
        cameraNode->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('S'))
        cameraNode->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('A'))
        cameraNode->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('D'))
        cameraNode->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);


	if (input->GetKeyPress(KEY_F2))
		GetSubsystem<DebugHud>()->ToggleAll();
}


void Game::SubscribeToEvents()
{
	SubscribeToEvent(E_UPDATE, HANDLER(Game, HandleUpdate));
	SubscribeToEvent(E_MOUSEBUTTONDOWN, HANDLER(Game, HandleMouseButtonDown));

}


void Game::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;
	float timeStep = eventData[P_TIMESTEP].GetFloat();
	MoveCamera(timeStep);

    if (pickedNode)
    {
        Vector3 hitPos;
        Drawable* hitDrawable;
        if (!Raycast(500, hitPos, hitDrawable, true))
            return;

        Vector3 pos = hitPos + deltaPickedNode;
        //pos.z_ = 0;
        pickedNode->SetPosition(pos);

        RigidBody2D* body = pickedNode->GetComponent<RigidBody2D>();
        //body->SetBodyType(BT_KINEMATIC);


        //body->SetAwake(false);
    }

}


void Game::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    Vector3 hitPos;
    Drawable* hitDrawable;
    if (!Raycast(500, hitPos, hitDrawable, false))
        return;
    pickedNode = hitDrawable->GetNode();
    RigidBody2D* body = pickedNode->GetComponent<RigidBody2D>();
    if (!body)
    {
        pickedNode = nullptr;
        return;
    }

    Vector3 hitPos2;
    Drawable* hitDrawable2;
    if (!Raycast(500, hitPos2, hitDrawable2, true))
    {
        pickedNode = nullptr;
        return;
    }

    //Vector3 pos = pickedNode->GetPosition();
    //pos.z_ = 0;

    deltaPickedNode = pickedNode->GetPosition() - hitPos2;
    //deltaPickedNode.z_ = 0;
    body->SetBodyType(BT_STATIC);

    //body->SetBodyType(BodyType2D::BT_KINEMATIC);
    //body->SetAwake(false);
    SubscribeToEvent(E_MOUSEBUTTONUP, HANDLER(Game, HandleMouseButtonUp));

}

void Game::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData)
{
    if (pickedNode)
    {
        RigidBody2D* body = pickedNode->GetComponent<RigidBody2D>();
        body->SetBodyType(BodyType2D::BT_DYNAMIC);

        float distanceX = pickedNode->GetPosition().x_ - startPoint.x_;
        float distanceY = pickedNode->GetPosition().y_ - startPoint.y_;

        float dist = (pickedNode->GetPosition() - startPoint).Length();
        float angle = Atan2(distanceY, distanceX);

        body->SetLinearVelocity(Vector2(-dist * Cos(angle), -dist * Sin(angle)) * 10);



        //body->SetAwake(true);
        pickedNode = NULL;
    }
    UnsubscribeFromEvent(E_MOUSEBUTTONUP);

}


bool Game::Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable, bool toInvisPlane)
{
    hitDrawable = 0;
    Ray cameraRay = GetCameraRay();
    PODVector<RayQueryResult> results;

    unsigned mask = 1;
    if (toInvisPlane)
        mask = 2;

    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY, mask);
    scene_->GetComponent<Octree>()->RaycastSingle(query);

    if (results.Size())
    {
        RayQueryResult& result = results[0];
        hitPos = result.position_;
        hitDrawable = result.drawable_;
        return true;
    }

    return false;
}


Ray Game::GetCameraRay()
{
    UI* ui = GetSubsystem<UI>();
    IntVector2 cursorPos = ui->GetCursorPosition();

    Node* cameraNode = scene_->GetChild("Camera");
    Camera* camera = cameraNode->GetComponent<Camera>();

    Graphics* graphics = GetSubsystem<Graphics>();

    return camera->GetScreenRay((float)cursorPos.x_ / graphics->GetWidth(), (float)cursorPos.y_ / graphics->GetHeight());
}
