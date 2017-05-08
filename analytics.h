#include <string>
#include <map>

namespace segment {

class Response {
public:
  Response();
  ~Response();

  bool ok;
  int status;
  std::string data;
};

typedef std::map<std::string, std::string> EventProperties;

typedef enum {
  EVENT_TYPE_IDENTIFY,
  EVENT_TYPE_TRACK,
  EVENT_TYPE_PAGE,
  EVENT_TYPE_SCREEN,
  EVENT_TYPE_GROUP,
  EVENT_TYPE_ALIAS
} EventType;



class Event {
public:
  Event (EventType type);
  ~Event ();

  std::string Serialize();
  std::string Type();

  std::string userId;
  std::string event;
  std::string groupId;
  std::string anonymousId;
  std::string previousId;
  EventProperties properties;

private:
  EventType type;
};

class Analytics {
public:
  Analytics (std::string writeKey);
  Analytics (std::string writeKey, std::string host);
  ~Analytics ();

  void Track (std::string userId, std::string event, EventProperties properties);
  void Identify (std::string userId, EventProperties traits);
  void Page (std::string event, std::string userId, EventProperties properties);
  void Screen (std::string event, std::string userId, EventProperties properties);
  void Alias (std::string previousId, std::string userId);
  void Group (std::string groupId, EventProperties properties);

private:
  std::string writeKey;
  std::string host;
  std::string userUage;

  Response *sendEvent (Event *e);
  static size_t sendEventWriteCallback (void *data, size_t size, size_t nmemb, void *userdata);
};

} // segment
