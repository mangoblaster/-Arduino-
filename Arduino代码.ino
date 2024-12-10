// 引脚定义
const int SENSOR_LEFT = 13;
const int SENSOR_CENTER = 12;
const int SENSOR_LEFT = 13;
const int SENSOR_CENTER = 12;
const int SENSOR_RIGHT = 11;

const int RIGHT_DIR_PIN1 = 2;
const int RIGHT_DIR_PIN2 = 4;
const int RIGHT_PWM_PIN = 3;

const int LEFT_DIR_PIN1 = 5;
const int LEFT_DIR_PIN2 = 7;
const int LEFT_PWM_PIN = 6;

// 枚举方向
enum Direction {
    LEFT_DIR,
    STRAIGHT,
    RIGHT_DIR,
    UNKNOWN
};

Direction lastDirection = Direction::STRAIGHT; // 初始设为直行
int baseSpeed = 150;
float Kp = 30.0, Ki = 0.0, Kd = 15.0;
float integral = 0;
float previous_error = 0;
unsigned long previous_time = 0;

void setup() {
    pinMode(SENSOR_LEFT, INPUT);
    pinMode(SENSOR_CENTER, INPUT);
    pinMode(SENSOR_RIGHT, INPUT);

    pinMode(RIGHT_DIR_PIN1, OUTPUT);
    pinMode(RIGHT_DIR_PIN2, OUTPUT);
    pinMode(RIGHT_PWM_PIN, OUTPUT);

    pinMode(LEFT_DIR_PIN1, OUTPUT);
    pinMode(LEFT_DIR_PIN2, OUTPUT);
    pinMode(LEFT_PWM_PIN, OUTPUT);

    // 根据实际情况修正方向，如果小车倒走，请调换HIGH/LOW逻辑
    digitalWrite(RIGHT_DIR_PIN1, LOW);
    digitalWrite(RIGHT_DIR_PIN2, HIGH);
    digitalWrite(LEFT_DIR_PIN1, LOW);
    digitalWrite(LEFT_DIR_PIN2, HIGH);

    Serial.begin(9600);
    previous_time = millis();
}

void loop() {
    bool left = digitalRead(SENSOR_LEFT);
    bool center = digitalRead(SENSOR_CENTER);
    bool right = digitalRead(SENSOR_RIGHT);

    // 判断是否是十字路口：三个传感器都检测到线
    if (left && center && right) {
        handleIntersection(left, center, right);
    }
    else {
        followLine(left, center, right);
    }

    delay(50);
}

void followLine(bool left, bool center, bool right) {
    int error = 0;

    if (left && !center && !right) {
        error = -1; // 左偏
        lastDirection = Direction::LEFT_DIR;
    }
    else if (!left && !center && right) {
        error = 1; // 右偏
        lastDirection = Direction::RIGHT_DIR;
    }
    else if (!left && center && !right) {
        error = 0; // 正中
        lastDirection = Direction::STRAIGHT;
    }
    else {
        // 遇到复杂情况（如T字路、断线），可根据实际需求处理。
        // 简单处理：保持上一次方向或减速
        error = 0;
    }

    unsigned long current_time = millis();
    float dt = (current_time - previous_time) / 1000.0;
    if (dt <= 0.0) dt = 0.0001;

    integral += error * dt;
    float derivative = (error - previous_error) / dt;
    float adjustment = Kp * error + Ki * integral + Kd * derivative;

    previous_error = error;
    previous_time = current_time;

    int leftSpeed = baseSpeed + adjustment;
    int rightSpeed = baseSpeed - adjustment;
    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);

    analogWrite(LEFT_PWM_PIN, leftSpeed);
    analogWrite(RIGHT_PWM_PIN, rightSpeed);

    Serial.print("Following Line | Error: ");
    Serial.print(error);
    Serial.print(" | Left Speed: ");
    Serial.print(leftSpeed);
    Serial.print(" | Right Speed: ");
    Serial.println(rightSpeed);
}

void handleIntersection(bool left, bool center, bool right) {
    // 简单逻辑：如果前方（中间）仍有线，则直行，不考虑转弯
    if (center) {
        goStraight();
        lastDirection = Direction::STRAIGHT;
        return;
    }

    // 如果前方没有线，但左、右有线，则需要根据lastDirection选择方向
    // 假设来时方向为straight，那么后方也可能检测到线，但我们不选后方，避免掉头
    // 简单规则：
    // - 优先尝试延续当前方向的趋势：如果上次是左偏，就尝试左转；如果上次是右偏，就尝试右转
    // - 如果上次是直行，那么可以指定一个优先策略（例如优先直行，没有则左转）
    // 注意：这里逻辑可根据实际需要增强。

    if (lastDirection == Direction::LEFT_DIR && left) {
        // 上次左偏，尝试左转
        turnLeft();
        lastDirection = Direction::LEFT_DIR;
    }
    else if (lastDirection == Direction::RIGHT_DIR && right) {
        // 上次右偏，尝试右转
        turnRight();
        lastDirection = Direction::RIGHT_DIR;
    }
    else {
        // 上次直行或无法按上次方向行走时，默认直行没有则尝试左转
        // （这里假设没有正前方线时的处理，可根据需要改进）
        if (left) {
            turnLeft();
            lastDirection = Direction::LEFT_DIR;
        }
        else if (right) {
            turnRight();
            lastDirection = Direction::RIGHT_DIR;
        }
        else {
            // 实在没有可走的路，就稍微倒车试试（应对复杂情况）
            backUp();
            // 倒车后再次尝试回到followLine状态
        }
    }
}

// 实现简单的转弯和直行函数
void goStraight() {
    analogWrite(LEFT_PWM_PIN, baseSpeed);
    analogWrite(RIGHT_PWM_PIN, baseSpeed);
    delay(300); // 稍微直行一段时间通过路口
    Serial.println("Go Straight through Intersection");
}

void turnLeft() {
    // 左转：左轮慢，右轮快
    analogWrite(LEFT_PWM_PIN, baseSpeed / 2);
    analogWrite(RIGHT_PWM_PIN, baseSpeed);
    delay(300);
    Serial.println("Turn Left at Intersection");
}

void turnRight() {
    // 右转：右轮慢，左轮快
    analogWrite(LEFT_PWM_PIN, baseSpeed);
    analogWrite(RIGHT_PWM_PIN, baseSpeed / 2);
    delay(300);
    Serial.println("Turn Right at Intersection");
}

void backUp() {
    // 倒车：两轮反转（根据实际电机方向引脚调节）
    digitalWrite(RIGHT_DIR_PIN1, HIGH);
    digitalWrite(RIGHT_DIR_PIN2, LOW);
    digitalWrite(LEFT_DIR_PIN1, HIGH);
    digitalWrite(LEFT_DIR_PIN2, LOW);

    analogWrite(LEFT_PWM_PIN, baseSpeed / 2);
    analogWrite(RIGHT_PWM_PIN, baseSpeed / 2);
    delay(400); // 倒车一段时间

    // 倒车结束后恢复方向
    digitalWrite(RIGHT_DIR_PIN1, LOW);
    digitalWrite(RIGHT_DIR_PIN2, HIGH);
    digitalWrite(LEFT_DIR_PIN1, LOW);
    digitalWrite(LEFT_DIR_PIN2, HIGH);

    Serial.println("Back Up and Try Again");
}

const int SENSOR_RIGHT = 11;

const int RIGHT_DIR_PIN1 = 2;
const int RIGHT_DIR_PIN2 = 4;
const int RIGHT_PWM_PIN = 3;

const int LEFT_DIR_PIN1 = 5;
const int LEFT_DIR_PIN2 = 7;
const int LEFT_PWM_PIN = 6;

// 枚举方向
enum Direction {
    LEFT_DIR,
    STRAIGHT,
    RIGHT_DIR,
    UNKNOWN
};

Direction lastDirection = Direction::STRAIGHT; // 初始设为直行
int baseSpeed = 150;
float Kp = 30.0, Ki = 0.0, Kd = 15.0;
float integral = 0;
float previous_error = 0;
unsigned long previous_time = 0;

void setup() {
    pinMode(SENSOR_LEFT, INPUT);
    pinMode(SENSOR_CENTER, INPUT);
    pinMode(SENSOR_RIGHT, INPUT);

    pinMode(RIGHT_DIR_PIN1, OUTPUT);
    pinMode(RIGHT_DIR_PIN2, OUTPUT);
    pinMode(RIGHT_PWM_PIN, OUTPUT);

    pinMode(LEFT_DIR_PIN1, OUTPUT);
    pinMode(LEFT_DIR_PIN2, OUTPUT);
    pinMode(LEFT_PWM_PIN, OUTPUT);

    // 根据实际情况修正方向，如果小车倒走，请调换HIGH/LOW逻辑
    digitalWrite(RIGHT_DIR_PIN1, LOW);
    digitalWrite(RIGHT_DIR_PIN2, HIGH);
    digitalWrite(LEFT_DIR_PIN1, LOW);
    digitalWrite(LEFT_DIR_PIN2, HIGH);

    Serial.begin(9600);
    previous_time = millis();
}

void loop() {
    bool left = digitalRead(SENSOR_LEFT);
    bool center = digitalRead(SENSOR_CENTER);
    bool right = digitalRead(SENSOR_RIGHT);

    // 判断是否是十字路口：三个传感器都检测到线
    if (left && center && right) {
        handleIntersection(left, center, right);
    }
    else {
        followLine(left, center, right);
    }

    delay(50);
}

void followLine(bool left, bool center, bool right) {
    int error = 0;

    if (left && !center && !right) {
        error = -1; // 左偏
        lastDirection = Direction::LEFT_DIR;
    }
    else if (!left && !center && right) {
        error = 1; // 右偏
        lastDirection = Direction::RIGHT_DIR;
    }
    else if (!left && center && !right) {
        error = 0; // 正中
        lastDirection = Direction::STRAIGHT;
    }
    else {
        // 遇到复杂情况（如T字路、断线），可根据实际需求处理。
        // 简单处理：保持上一次方向或减速
        error = 0;
    }

    unsigned long current_time = millis();
    float dt = (current_time - previous_time) / 1000.0;
    if (dt <= 0.0) dt = 0.0001;

    integral += error * dt;
    float derivative = (error - previous_error) / dt;
    float adjustment = Kp * error + Ki * integral + Kd * derivative;

    previous_error = error;
    previous_time = current_time;

    int leftSpeed = baseSpeed + adjustment;
    int rightSpeed = baseSpeed - adjustment;
    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);

    analogWrite(LEFT_PWM_PIN, leftSpeed);
    analogWrite(RIGHT_PWM_PIN, rightSpeed);

    Serial.print("Following Line | Error: ");
    Serial.print(error);
    Serial.print(" | Left Speed: ");
    Serial.print(leftSpeed);
    Serial.print(" | Right Speed: ");
    Serial.println(rightSpeed);
}

void handleIntersection(bool left, bool center, bool right) {
    // 简单逻辑：如果前方（中间）仍有线，则直行，不考虑转弯
    if (center) {
        goStraight();
        lastDirection = Direction::STRAIGHT;
        return;
    }

    // 如果前方没有线，但左、右有线，则需要根据lastDirection选择方向
    // 假设来时方向为straight，那么后方也可能检测到线，但我们不选后方，避免掉头
    // 简单规则：
    // - 优先尝试延续当前方向的趋势：如果上次是左偏，就尝试左转；如果上次是右偏，就尝试右转
    // - 如果上次是直行，那么可以指定一个优先策略（例如优先直行，没有则左转）
    // 注意：这里逻辑可根据实际需要增强。

    if (lastDirection == Direction::LEFT_DIR && left) {
        // 上次左偏，尝试左转
        turnLeft();
        lastDirection = Direction::LEFT_DIR;
    }
    else if (lastDirection == Direction::RIGHT_DIR && right) {
        // 上次右偏，尝试右转
        turnRight();
        lastDirection = Direction::RIGHT_DIR;
    }
    else {
        // 上次直行或无法按上次方向行走时，默认直行没有则尝试左转
        // （这里假设没有正前方线时的处理，可根据需要改进）
        if (left) {
            turnLeft();
            lastDirection = Direction::LEFT_DIR;
        }
        else if (right) {
            turnRight();
            lastDirection = Direction::RIGHT_DIR;
        }
        else {
            // 实在没有可走的路，就稍微倒车试试（应对复杂情况）
            backUp();
            // 倒车后再次尝试回到followLine状态
        }
    }
}

// 实现简单的转弯和直行函数
void goStraight() {
    analogWrite(LEFT_PWM_PIN, baseSpeed);
    analogWrite(RIGHT_PWM_PIN, baseSpeed);
    delay(300); // 稍微直行一段时间通过路口
    Serial.println("Go Straight through Intersection");
}

void turnLeft() {
    // 左转：左轮慢，右轮快
    analogWrite(LEFT_PWM_PIN, baseSpeed / 2);
    analogWrite(RIGHT_PWM_PIN, baseSpeed);
    delay(300);
    Serial.println("Turn Left at Intersection");
}

void turnRight() {
    // 右转：右轮慢，左轮快
    analogWrite(LEFT_PWM_PIN, baseSpeed);
    analogWrite(RIGHT_PWM_PIN, baseSpeed / 2);
    delay(300);
    Serial.println("Turn Right at Intersection");
}

void backUp() {
    // 倒车：两轮反转（根据实际电机方向引脚调节）
    digitalWrite(RIGHT_DIR_PIN1, HIGH);
    digitalWrite(RIGHT_DIR_PIN2, LOW);
    digitalWrite(LEFT_DIR_PIN1, HIGH);
    digitalWrite(LEFT_DIR_PIN2, LOW);

    analogWrite(LEFT_PWM_PIN, baseSpeed / 2);
    analogWrite(RIGHT_PWM_PIN, baseSpeed / 2);
    delay(400); // 倒车一段时间

    // 倒车结束后恢复方向
    digitalWrite(RIGHT_DIR_PIN1, LOW);
    digitalWrite(RIGHT_DIR_PIN2, HIGH);
    digitalWrite(LEFT_DIR_PIN1, LOW);
    digitalWrite(LEFT_DIR_PIN2, HIGH);

    Serial.println("Back Up and Try Again");
}
