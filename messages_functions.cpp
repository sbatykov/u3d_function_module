#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <vector>
#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "messages_functions.h"

/// MACROS
#define GET_COORDS(extract_function)              \
  bool is_exist = false;                          \
  (*box_mutex).lock();                            \
  for (auto i = ids_of_created_objects->begin();  \
       i != ids_of_created_objects->end(); i++) { \
    if ((*i) == object_id) {                      \
      (*box_mutex).unlock();                      \
      is_exist = true;                            \
      std::string params("+obj:coords,");         \
      params.append(returnStr(object_id));        \
      std::string temp_string;                    \
      temp_string = createMessage(params);        \
      double d = extract_function(temp_string);   \
      return d;                                   \
    }                                             \
  }                                               \
  if (!is_exist) {                                \
    (*box_mutex).unlock();                        \
    throw std::exception();                       \
  }

#define DELETE_OBJECT_MACRO                       \
  bool is_exist = false;                          \
  (*box_mutex).lock();                            \
  for (auto i = ids_of_created_objects->begin();  \
       i != ids_of_created_objects->end(); i++) { \
    if ((*i) == object_id) {                      \
      (*box_mutex).unlock();                      \
      is_exist = true;                            \
      std::string params("+delete:");             \
      params.append(returnStr(object_id));        \
      createMessage(params);                      \
      (*box_mutex).lock();                        \
      ids_of_created_objects->erase(i);           \
      (*box_mutex).unlock();                      \
      break;                                      \
    }                                             \
  }

//// Variables
bool is_read_from_shared_memory = false;

// Box OfMessages
std::vector<BoxOfMessagesData *> *box_of_messages;
boost::mutex *box_mutex;
boost::condition_variable *postman_thread_waker;
bool *postman_thread_waker_flag;
bool *is_world_initialized;
std::vector<int> *ids_of_created_objects;

std::string returnStr(int _i) {
  char buf[4 * sizeof(int)];
  const int len = sprintf(buf, "%d", _i);
  std::string temp_str(buf, len);
  return temp_str;
};

double extractString(std::string str, char first, char second) {
  std::string temp("");

  int beg = 0;
  int end = 0;
  int i = 1;
  if (first == '%') {
    i++;
  }
  beg = str.find(first) + i;
  end = str.find(second);

  temp.assign(str, beg, end - beg);

  return strtod(temp.c_str(), NULL);
};
int extractObj_id(std::string str) {
  return (int)extractString(str, ':', '&');
};
double extractX(std::string str) { return extractString(str, ':', ','); };
double extractY(std::string str) {
  int first_pos = str.find(',') + 1;
  int last_pos = str.find(',', first_pos + 1);
  std::string temp_str = str.substr(first_pos, last_pos - first_pos);

  return strtod(temp_str.c_str(), NULL);
};
double extractZ(std::string str) {
  int first_pos = str.find(',');
  first_pos = str.find(',', first_pos + 1) + 1;
  int last_pos = str.find(',', first_pos + 1);
  std::string temp_str = str.substr(first_pos, last_pos - first_pos);
  return strtod(temp_str.c_str(), NULL);
};
double extractAngle(std::string str) {
  int first_pos = str.find_last_of(',') + 1;
  int last_pos = str.find('&');
  std::string temp_str = str.substr(first_pos, last_pos - first_pos);
  return strtod(temp_str.c_str(), NULL);
};
std::string extractMessage(std::string str) {
  std::string temp("");

  char first = '+';
  char second = '&';

  unsigned int beg = 0;
  unsigned int end = 0;

  beg = str.find(first) + 1;
  end = str.find(second) + 1;

  temp.assign(str, beg, end - beg);
  return temp;
};

int extractUniq_Id(std::string str) {
  return (int)extractString(str, '%', '+');
};

void testStringSuccess(std::string str) {
  if (str.find("fail") != std::string::npos) {
    throw std::exception();
  }
};

void readSharedMemory() {
  boost::interprocess::shared_memory_object shm_obj(
      boost::interprocess::open_only, "PostmansSharedMemory",
      boost::interprocess::read_write);
  boost::interprocess::mapped_region region(shm_obj,
                                            boost::interprocess::read_only);

  MutexAndBoxVector **mtx_and_box_vector =
      static_cast<MutexAndBoxVector **>(region.get_address());
  box_mutex = (*mtx_and_box_vector)->mtx;
  box_of_messages = (*mtx_and_box_vector)->box;
  postman_thread_waker = (*mtx_and_box_vector)->cond_postman_thread_waker;
  postman_thread_waker_flag =
      (*mtx_and_box_vector)->bool_postman_thread_waker_flag;
  is_world_initialized = (*mtx_and_box_vector)->is_world_initialized_flag;
  ids_of_created_objects = (*mtx_and_box_vector)->ids_of_objects;
  is_read_from_shared_memory = true;
}

std::string createMessage(std::string params) {
  if (!is_read_from_shared_memory) {
    readSharedMemory();
  }

  boost::mutex message_mutex;
  boost::condition_variable wait_recieved_message;
  BoxOfMessagesData *message_struct =
      new BoxOfMessagesData(&wait_recieved_message, false, params);
  // wake up postman thread
  (*box_mutex).lock();
  box_of_messages->push_back(message_struct);
  (*postman_thread_waker_flag) = true;
  (*postman_thread_waker).notify_one();
  (*box_mutex).unlock();

  // wait to recieve socket answer
  boost::unique_lock<boost::mutex> lock(message_mutex);
  while (!message_struct->bool_messenger_wake_flag) {
    (*(message_struct->cond_messenger_waker)).wait(lock);
  }

  std::string result_message_str(message_struct->_message);
  delete message_struct;

  testStringSuccess(result_message_str);
  return result_message_str;
};

// createWorld
void createWorld(int x, int y, int z) {
  std::string params("+init:");
  params.append(returnStr(x));
  params.append(",");
  params.append(returnStr(y));
  params.append(",");
  params.append(returnStr(z));
  createMessage(params);
}

// destroyWorld
void destroyWorld() {
  std::string params("+destroy");
  createMessage(params);
  (*box_mutex).lock();
  ids_of_created_objects->clear();
  (*box_mutex).unlock();
}

// deleteObject
void deleteObject(int object_id) {
  DELETE_OBJECT_MACRO
  // Throw exception if can't find element among existing
  if (!is_exist) {
    (*box_mutex).unlock();
    throw std::exception();
  }
}

// deleteRobot
void deleteRobot(int object_id) {
  DELETE_OBJECT_MACRO
  if (!is_exist) {
    (*box_mutex).unlock();
  }
}

// createCube
int createCube(int x, int y, int z, int dx, int dy, int dz, int angle, int hold,
               std::string color) {
  std::string params("+obj:cube,");
  params.append(returnStr(x));
  params.append(",");
  params.append(returnStr(y));
  params.append(",");
  params.append(returnStr(z));
  params.append(",");
  params.append(returnStr(dx));
  params.append(",");
  params.append(returnStr(dy));
  params.append(",");
  params.append(returnStr(dz));
  params.append(",");
  params.append(returnStr(angle));
  params.append(",");
  params.append(returnStr(hold));
  params.append(",");
  params.append(color);

  std::string temp_string;
  temp_string = createMessage(params);
  int d = extractObj_id(temp_string);
  //// insert id of created object
  (*box_mutex).lock();
  ids_of_created_objects->push_back(d);
  (*box_mutex).unlock();
  return d;
}

// createSphere
int createSphere(int x, int y, int z, int R, int hold, std::string color) {
  std::string params("+obj:sphere,");
  params.append(returnStr(x));
  params.append(",");
  params.append(returnStr(y));
  params.append(",");
  params.append(returnStr(z));
  params.append(",");
  params.append(returnStr(R));
  params.append(",");
  params.append(returnStr(hold));
  params.append(",");
  params.append(color);

  std::string temp_string;
  temp_string = createMessage(params);
  int d = extractObj_id(temp_string);
  //// insert id of created object
  (*box_mutex).lock();
  ids_of_created_objects->push_back(d);
  (*box_mutex).unlock();
  return d;
}

// createModel
int createModel(int x, int y, int z, int scale_x, int scale_y, int scale_z,
                int angle, int hold, std::string color, std::string path) {
  std::string params("+obj:model,");
  params.append(returnStr(x));
  params.append(",");
  params.append(returnStr(y));
  params.append(",");
  params.append(returnStr(z));
  params.append(",");
  params.append(returnStr(scale_x));
  params.append(",");
  params.append(returnStr(scale_y));
  params.append(",");
  params.append(returnStr(scale_z));
  params.append(",");
  params.append(returnStr(angle));
  params.append(",");
  params.append(returnStr(hold));
  params.append(",");
  params.append(color);
  params.append(",");
  params.append(path);

  std::string temp_string;
  temp_string = createMessage(params);
  int d = extractObj_id(temp_string);
  //// insert id of created object
  (*box_mutex).lock();
  ids_of_created_objects->push_back(d);
  (*box_mutex).unlock();
  return d;
}

// changeColor
void changeColor(int object_id, std::string color) {
  bool is_exist = false;
  (*box_mutex).lock();
  for (auto i = ids_of_created_objects->begin();
       i != ids_of_created_objects->end(); i++) {
    if ((*i) == object_id) {
      (*box_mutex).unlock();
      is_exist = true;
      std::string params("+obj:color,");
      params.append(returnStr(object_id));
      params.append(",");
      params.append(color);

      createMessage(params);
      break;
    }
  }
  // Throw exception if can't find element among existing
  if (!is_exist) {
    (*box_mutex).unlock();
    throw std::exception();
  }
}

// moveObject
void moveObject(int object_id, int x, int y, int z, int angle, int speed_coord,
                int speed_angle) {
  bool is_exist = false;
  (*box_mutex).lock();
  for (auto i = ids_of_created_objects->begin();
       i != ids_of_created_objects->end(); i++) {
    if ((*i) == object_id) {
      (*box_mutex).unlock();
      is_exist = true;
      std::string params("+obj:move,");
      params.append(returnStr(object_id));
      params.append(",");
      params.append(returnStr(x));
      params.append(",");
      params.append(returnStr(y));
      params.append(",");
      params.append(returnStr(z));
      params.append(",");
      params.append(returnStr(angle));
      params.append(",");
      params.append(returnStr(speed_coord));
      params.append(",");
      params.append(returnStr(speed_angle));

      createMessage(params);
      break;
    }
  }
  // Throw exception if can't find element among existing
  if (!is_exist) {
    (*box_mutex).unlock();
    throw std::exception();
  }
}

// changeStatus
void changeStatus(int object_id, int hold) {
  bool is_exist = false;
  (*box_mutex).lock();
  for (auto i = ids_of_created_objects->begin();
       i != ids_of_created_objects->end(); i++) {
    if ((*i) == object_id) {
      (*box_mutex).unlock();
      is_exist = true;
      std::string params("+obj:hold,");
      params.append(returnStr(object_id));
      params.append(",");
      params.append(returnStr(hold));

      createMessage(params);
      break;
    }
  }
  // Throw exception if can't find element among existing
  if (!is_exist) {
    (*box_mutex).unlock();
    throw std::exception();
  }
}

// getX
double getX(int object_id) { GET_COORDS(extractX); }

// getY
double getY(int object_id) { GET_COORDS(extractY); }

// getZ
double getZ(int object_id) { GET_COORDS(extractZ); }

// getAngle
double getAngle(int object_id) { GET_COORDS(extractAngle); }
