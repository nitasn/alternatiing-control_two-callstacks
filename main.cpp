#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv_worker, cv_main;
bool algorithm_can_proceed = false;
bool main_can_proceed = false;
bool algorithm_has_terminated = false;

void algorithm();


void pause_algorithm() {
  std::unique_lock<std::mutex> lock(mtx);

  main_can_proceed = true;
  cv_main.notify_one();

  algorithm_can_proceed = false;
  cv_worker.wait(lock, []{ return algorithm_can_proceed; });
}

void resume_algorithm() {
  std::unique_lock<std::mutex> lock(mtx);

  algorithm_can_proceed = true;
  cv_worker.notify_one();
  
  main_can_proceed = false;
  cv_main.wait(lock, []{ return main_can_proceed; });
}

void init_algorithm_thread() {
  auto cleanup = [] {
    std::unique_lock<std::mutex> lock(mtx);
    algorithm_has_terminated = true;

    main_can_proceed = true;
    cv_main.notify_one();
  };

  std::thread([&cleanup] {
    pause_algorithm();
    algorithm();
    cleanup();
  }).detach();
}

///////////////////////////////////////////////////////////////////////////

int current_degrees = 0;
int current_pos_x = 0;

void rotate_to_degrees(int dest_degrees) {
  while (current_degrees != dest_degrees) {
    int degrees_step = current_degrees < dest_degrees ? +30 : -30;
    std::cout << "rotating " << degrees_step << " degrees" << '\n';
    current_degrees += degrees_step;
    pause_algorithm();
  }
}

void drive_straight_distance(int distance) {
  int dest_pos_x = current_pos_x + distance;
  while (current_pos_x < dest_pos_x) {
    std::cout << "advancing 1cm" << '\n';
    current_pos_x += 1;
    pause_algorithm();
  }
}

void algorithm() {
  rotate_to_degrees(90);
  drive_straight_distance(4);
  rotate_to_degrees(180);
  drive_straight_distance(2);
}

///////////////////////////////////////////////////////////////////////////

void setup() {
  init_algorithm_thread();
}

void loop() {
  if (!algorithm_has_terminated)
    resume_algorithm();
}

int main() {
  setup();

  while (!algorithm_has_terminated) {
    loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
