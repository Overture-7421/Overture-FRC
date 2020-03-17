#include "Chassis.h"
#include "commands/TeleopDrive/TeleopDrive.h"

Chassis::Chassis()
{
    leftMaster.ConfigFactoryDefault();
    rightMaster.ConfigFactoryDefault();

    gyro.Reset();
    gyro.ZeroYaw();
    gyro.ResetDisplacement();
    
    frc::SmartDashboard::PutData("Chassis/Chassis", this);
    leftMaster.ConfigSelectedFeedbackSensor(FeedbackDevice::CTRE_MagEncoder_Relative);
    rightMaster.ConfigSelectedFeedbackSensor(FeedbackDevice::CTRE_MagEncoder_Relative);

    rightMaster.ConfigSelectedFeedbackCoefficient(1.0 / 4.0);
    leftMaster.ConfigSelectedFeedbackCoefficient(1.0 / 4.0);

    rightMotor1.Follow(rightMaster);
    rightMotor2.Follow(rightMaster);

    leftMotor1.Follow(leftMaster);
    leftMotor2.Follow(leftMaster);

    leftMaster.ConfigOpenloopRamp(ChassisMap::RAMP_RATE);
    rightMaster.ConfigOpenloopRamp(ChassisMap::RAMP_RATE);
    rightMaster.SetInverted(true);
    rightMotor1.SetInverted(true);
    rightMotor2.SetInverted(true);
    rightMaster.ConfigPeakCurrentLimit(ChassisMap::peakCurrentLimit);
    rightMaster.ConfigContinuousCurrentLimit(ChassisMap::continuousCurrentLimit);
    leftMaster.ConfigContinuousCurrentLimit(ChassisMap::continuousCurrentLimit);
    rightMaster.ConfigPeakCurrentLimit(ChassisMap::peakCurrentLimit);
    leftMaster.SetSelectedSensorPosition(0);
    rightMaster.SetSelectedSensorPosition(0);
    leftMaster.SetNeutralMode(NeutralMode::Brake);
    rightMaster.SetNeutralMode(NeutralMode::Brake);
    rightMotor1.SetNeutralMode(NeutralMode::Brake);
    rightMotor2.SetNeutralMode(NeutralMode::Brake);
    leftMotor1.SetNeutralMode(NeutralMode::Brake);
    leftMotor2.SetNeutralMode(NeutralMode::Brake);
}

void Chassis::Periodic()
{
    frc::SmartDashboard::PutNumber("Chassis/X", double_t(odometry.GetPose().Translation().X()));
    frc::SmartDashboard::PutNumber("Chassis/Y", double_t(odometry.GetPose().Translation().Y()));
    frc::SmartDashboard::PutNumber("Chassis/yaw", -gyro.GetYaw());
    frc::SmartDashboard::PutNumber("Chassis/pitch", gyro.GetPitch());
    frc::SmartDashboard::PutNumber("Chassis/roll", gyro.GetRoll());
    frc::SmartDashboard::PutNumber("Chassis/heading", -gyro.GetFusedHeading());
    frc::SmartDashboard::PutNumber("Chassis/Left Position", leftMaster.GetSelectedSensorPosition());
    frc::SmartDashboard::PutNumber("Chassis/Right Position", rightMaster.GetSelectedSensorPosition());
    frc::SmartDashboard::PutNumber("Chassis/Left Speed", leftMaster.GetSelectedSensorVelocity());
    frc::SmartDashboard::PutNumber("Chassis/Right Speed", rightMaster.GetSelectedSensorVelocity());
    frc::SmartDashboard::PutNumber("Chassis/Left Master Current", leftMaster.GetSupplyCurrent());
    frc::SmartDashboard::PutNumber("Chassis/ Right Master Current", rightMaster.GetSupplyCurrent());
    frc::Rotation2d gyroRot2d = frc::Rotation2d(units::degree_t(-gyro.GetAngle()));
    odometry.Update(gyroRot2d, units::meter_t(leftMaster.GetSelectedSensorPosition() * ChassisMap::ENC_METER_PER_PULSE),
                    units::meter_t(rightMaster.GetSelectedSensorPosition() * ChassisMap::ENC_METER_PER_PULSE));
}
frc::DifferentialDriveWheelSpeeds Chassis::getWheelSpeeds()
{
    frc::DifferentialDriveWheelSpeeds wheelspeeds;
    wheelspeeds.left = units::meters_per_second_t(leftMaster.GetSelectedSensorVelocity() * ChassisMap::ENC_METER_PER_PULSE / 0.1);
    wheelspeeds.right = units::meters_per_second_t(rightMaster.GetSelectedSensorVelocity() * ChassisMap::ENC_METER_PER_PULSE / 0.1);
    return wheelspeeds;
}
void Chassis::arcadeDrive(double linear, double angular)
{
    leftMaster.Set(linear - angular);
    rightMaster.Set(linear + angular);
}

void Chassis::tankDrive(double leftSpeed, double rightSpeed)
{
    leftMaster.Set(leftSpeed);
    rightMaster.Set(rightSpeed);
}

void Chassis::voltageDrive(units::volt_t leftVoltage, units::volt_t rightVoltage)
{
    leftMaster.SetVoltage(leftVoltage);
    rightMaster.SetVoltage(rightVoltage);
    frc::SmartDashboard::PutNumber("RightVoltage", double_t(rightVoltage));
    frc::SmartDashboard::PutNumber("LeftVoltage", double_t(leftVoltage));
}

double Chassis::getYaw()
{
    return gyro.GetYaw();
}

frc2::SequentialCommandGroup Chassis::getRamsetteCommand(const Pose2d &start, const std::vector<Translation2d> &interiorWaypoints, const Pose2d &end, bool reversed, double maxSpeed)
{
    //gyro.SetAngleAdjustment(180);
    frc::DifferentialDriveVoltageConstraint autoVoltageConstraint(
        frc::SimpleMotorFeedforward<units::meters>(
            ChassisMap::ks, ChassisMap::kv, ChassisMap::ka),
        kinematics, 10_V);

    // Set up config for trajectory
    frc::TrajectoryConfig config(maxSpeed != -1 ? units::meters_per_second_t(maxSpeed) : ChassisMap::kMaxSpeed,
                                 ChassisMap::kMaxAcceleration);
    config.SetReversed(reversed);
    // Add kinematics to ensure max speed is actually obeyed
    config.SetKinematics(kinematics);
    // Apply the voltage constraint
    config.AddConstraint(autoVoltageConstraint);
    auto targetTrajectory = frc::TrajectoryGenerator::GenerateTrajectory(start, interiorWaypoints, end, config);
    auto ramseteController = frc::RamseteController(ChassisMap::kRamseteB,
                                                    ChassisMap::kRamseteZeta);
    auto commandGroup = frc2::SequentialCommandGroup();
    commandGroup.AddCommands(
        frc2::RamseteCommand(
            targetTrajectory, [this]() { return getPose(); },
            ramseteController,
            frc::SimpleMotorFeedforward<units::meters>(
                ChassisMap::ks, ChassisMap::kv, ChassisMap::ka),
            kinematics,
            [this] { return getWheelSpeeds(); },
            frc2::PIDController(ChassisMap::kPDriveVel, 0, 0),
            frc2::PIDController(ChassisMap::kPDriveVel, 0, 0),
            [this](auto left, auto right) {
                voltageDrive(left, right);
            },
            {this}),
        frc2::InstantCommand(
            [this]() { voltageDrive(0_V, 0_V); }, this));
    return commandGroup;
}

frc::Pose2d Chassis::getPose()
{

    return odometry.GetPose();
}
void Chassis::setRateLimit(bool state){
    if(state){
        leftMaster.ConfigOpenloopRamp(ChassisMap::RAMP_RATE);
        rightMaster.ConfigOpenloopRamp(ChassisMap::RAMP_RATE);
    }else{
        leftMaster.ConfigOpenloopRamp(0.0);
        rightMaster.ConfigOpenloopRamp(0.0);
    }
}
