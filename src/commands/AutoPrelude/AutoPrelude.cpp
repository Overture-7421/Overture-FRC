/*----------------------------------------------------------------------------*/
/* Copyright (c) 2019 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "AutoPrelude.h"

// NOTE:  Consider using this command inline, rather than writing a subclass.
// For more information, see:
// https://docs.wpilib.org/en/latest/docs/software/commandbased/convenience-features.html
AutoPrelude::AutoPrelude(Chassis* chassis) {
  // Add your commands here, e.g.
  
   AddCommands(
      chassis->getRamsetteCommand(
       frc::Pose2d(0_m,0_m,frc::Rotation2d(0_deg)),
       {/*Empty vec*/},
       frc::Pose2d(2_m,-1_m,frc::Rotation2d(-90_deg))
   )
   ,AlignToTower(chassis)
   );
}
