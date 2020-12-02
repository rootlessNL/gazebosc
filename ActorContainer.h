#ifndef ACTORCONTAINER_H
#define ACTORCONTAINER_H

#include "libsphactor.h"
#include <string>
#include <vector>
#include "ImNodes.h"
#include "ImNodesEz.h"
#include <czmq.h>

/// A structure defining a connection between two slots of two actors.
struct Connection
{
    /// `id` that was passed to BeginNode() of input node.
    void* input_node = nullptr;
    /// Descriptor of input slot.
    const char* input_slot = nullptr;
    /// `id` that was passed to BeginNode() of output node.
    void* output_node = nullptr;
    /// Descriptor of output slot.
    const char* output_slot = nullptr;

    bool operator==(const Connection& other) const
    {
        return input_node == other.input_node &&
               input_slot == other.input_slot &&
               output_node == other.output_node &&
               output_slot == other.output_slot;
    }

    bool operator!=(const Connection& other) const
    {
        return !operator ==(other);
    }
};

enum GActorSlotTypes
{
    ActorSlotAny = 1,    // ID can not be 0
    ActorSlotPosition,
    ActorSlotRotation,
    ActorSlotMatrix,
    ActorSlotInt,
    ActorSlotOSC
};

struct ActorContainer {
  /// Default value UI strings will have if no maximum is set
  const int MAX_STR_DEFAULT = 256;
  /// Title which will be displayed at the center-top of the actor.
  const char* title = nullptr;
  /// Flag indicating that actor is selected by the user.
  bool selected = false;
  /// Actor position on the canvas.
  ImVec2 pos{};
  /// List of actor connections.
  std::vector<Connection> connections{};
  /// A list of input slots current actor has.
  std::vector<ImNodes::Ez::SlotInfo> input_slots{};
  /// A list of output slots current actor has.
  std::vector<ImNodes::Ez::SlotInfo> output_slots{};

  sphactor_t *actor;
  zconfig_t *capabilities;

  ActorContainer(sphactor_t *actor) {
    this->actor = actor;
    this->title = sphactor_ask_actor_type(actor);
    this->capabilities = zconfig_dup(sphactor_ask_capability(actor));

    ParseConnections();

    //TODO: Perform this based on capabilities?
    //          Move to the instance constructor?
    sphactor_ask_set_timeout(actor, 16);
  }

  ~ActorContainer() {
    zconfig_destroy(&this->capabilities);
  }

  void ParseConnections() {
    if ( this->capabilities == NULL ) return;

    zconfig_t *inputs = zconfig_locate(this->capabilities, "inputs");
    zconfig_t *outputs = zconfig_locate(this->capabilities, "outputs");

    Parse(inputs, "input", &input_slots);
    Parse(outputs, "output", &output_slots);
  }

  void Parse(zconfig_t * config, const char* node, std::vector<ImNodes::Ez::SlotInfo> *list ) {
    if ( config == NULL ) return;

    zconfig_t *localNode = zconfig_locate( config, node);
    while ( localNode != NULL ) {
        zconfig_t *type = zconfig_locate(localNode, "type");
        assert(type);

        char* typeStr = zconfig_value(type);
        if ( streq(typeStr, "OSC")) {
            list->push_back({ "OSC", ActorSlotOSC });
        }
        else {
            zsys_error("Unsupported %s: %s", node, typeStr);
        }

        localNode = zconfig_next(localNode);
    }
  }

  void Render(float deltaTime) {
    //loop through each data element in capabilities
    if ( this->capabilities == NULL ) return;

    zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
    if ( root == NULL ) return;

    zconfig_t *data = zconfig_locate(root, "data");
    while( data != NULL ) {
        zconfig_t *name = zconfig_locate(data, "name");
        zconfig_t *type = zconfig_locate(data, "type");
        assert(name);
        assert(type);

        char* nameStr = zconfig_value(name);
        char* typeStr = zconfig_value(type);
        if ( streq(typeStr, "int")) {
            RenderInt( nameStr, data );
        }
        else if ( streq(typeStr, "float")) {
            RenderFloat( nameStr, data );
        }
        else if ( streq(typeStr, "string")) {
            RenderString( nameStr, data );
        }

        data = zconfig_next(data);
    }
  }

  void RenderInt(const char* name, zconfig_t *data) {
    int value;
    int min = 0, max = 0, step = 0;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    assert(zvalue);
    assert(zapic);

    zconfig_t * zmin = zconfig_locate(data, "min");
    zconfig_t * zmax = zconfig_locate(data, "max");
    zconfig_t * zstep = zconfig_locate(data, "step");

    ReadInt( &value, zvalue);
    ReadInt( &min, zmin);
    ReadInt( &max, zmax);
    ReadInt( &step, zstep);

    ImGui::SetNextItemWidth(100);
    if ( ImGui::InputInt( name, &value, step) ) {
        if ( min != max ) {
            if ( value < min ) value = min;
            if ( value > max ) value = max;
        }
        zconfig_set_value(zvalue, "%i", value);
        if (zapiv)
        {
            std::string pic = "s";
            pic += zconfig_value(zapiv);
            zsock_send( sphactor_socket(this->actor), pic.c_str(), zconfig_value(zapic), value);
        }
        else
            zsock_send( sphactor_socket(this->actor), "si", zconfig_value(zapic), value);
    }
  }

  void RenderFloat(const char* name, zconfig_t *data) {
      float value;
      float min = 0, max = 0, step = 0;

      zconfig_t * zvalue = zconfig_locate(data, "value");
      assert(zvalue);

      zconfig_t * zmin = zconfig_locate(data, "min");
      zconfig_t * zmax = zconfig_locate(data, "max");
      zconfig_t * zstep = zconfig_locate(data, "step");

      ReadFloat( &value, zvalue);
      ReadFloat( &min, zmin);
      ReadFloat( &max, zmax);
      ReadFloat( &step, zstep);

      ImGui::SetNextItemWidth(100);
      if ( min != max ) {
          if ( ImGui::SliderFloat( name, &value, min, max) ) {
              zconfig_set_value(zvalue, "%f", value);
          }
      }
      else {
          if ( ImGui::InputFloat( name, &value, min, max) ) {
              zconfig_set_value(zvalue, "%f", value);
          }
      }
  }

  void RenderString(const char* name, zconfig_t *data) {
    int max = MAX_STR_DEFAULT;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    assert(zvalue);
    zconfig_t * zmax = zconfig_locate(data, "max");

    ReadInt( &max, zmax );

    char* buf = new char[MAX_STR_DEFAULT];
    const char* zvalueStr = zconfig_value(zvalue);
    strcpy(buf, zvalueStr);

    ImGui::SetNextItemWidth(200);
    if ( ImGui::InputText(name, buf, max ) ) {
        zconfig_set_value(zvalue, "%s", buf);
    }

    free(buf);
  }

  void ReadInt( int *value, zconfig_t * data) {
    if ( data != NULL ) {
        *value = atoi(zconfig_value(data));
    }
  }

  void ReadFloat( float *value, zconfig_t * data) {
    if ( data != NULL ) {
        *value = atof(zconfig_value(data));
    }
  }

  void CreateActor() {
  }

  void DestroyActor() {
    sphactor_destroy(&actor);
  }

  void DeleteConnection(const Connection& connection)
  {
      for (auto it = connections.begin(); it != connections.end(); ++it)
      {
          if (connection == *it)
          {
              connections.erase(it);
              break;
          }
      }
  }

  //TODO: Add custom report data to this?
  void SerializeActorData(zconfig_t *section) {
      zconfig_t *xpos = zconfig_new("xpos", section);
      zconfig_set_value(xpos, "%f", pos.x);

      zconfig_t *ypos = zconfig_new("ypos", section);
      zconfig_set_value(ypos, "%f", pos.y);
  }

  void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it) {
      char* xpos = *it;
      it++;
      char* ypos = *it;
      it++;

      pos.x = atof(xpos);
      pos.y = atof(ypos);

      free(xpos);
      free(ypos);
  }
};

#endif