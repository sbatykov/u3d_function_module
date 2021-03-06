#ifndef MESSAGES_FUNCTIONS_H
#define MESSAGES_FUNCTIONS_H

struct BoxOfMessagesData;
struct MutexAndBoxVector;

struct BoxOfMessagesData {
  boost::condition_variable *cond_messenger_waker;
  bool bool_messenger_wake_flag;
  std::string _message;

  BoxOfMessagesData(boost::condition_variable *cond_,
                    bool bool_messenger_wake_flag, std::string _message)
      : bool_messenger_wake_flag(bool_messenger_wake_flag), _message(_message) {
    this->cond_messenger_waker = cond_;
  };
};
struct MutexAndBoxVector {
  boost::mutex *mtx;
  std::vector<BoxOfMessagesData *> *box;
  boost::condition_variable *cond_postman_thread_waker;
  bool *bool_postman_thread_waker_flag;
  bool *is_world_initialized_flag;
  std::vector<int> *ids_of_objects;
};

/// Helper Functions
std::string returnStr(int _i);
int extractUniq_Id(std::string str);
void readSharedMemory();
///

void createWorld(int x, int y, int z);
void destroyWorld();
void deleteObject(int object_id);
void deleteRobot(int object_id);
int createCube(int x, int y, int z, int dx, int dy, int dz, int angle, int hold,
               std::string color);
int createSphere(int x, int y, int z, int R, int hold, std::string color);

int createModel(int x, int y, int z, int scale_x, int scale_y, int scale_z,
                int angle, int hold, std::string color, std::string path);

void changeColor(int object_id, std::string color);
void moveObject(int object_id, int x, int y, int z, int angle, int speed_coord,
                int speed_angle);
void changeStatus(int object_id, int hold);
double getX(int object_id);
double getY(int object_id);
double getZ(int object_id);
double getAngle(int object_id);

#endif /* MESSAGES_FUNCTIONS_H  */