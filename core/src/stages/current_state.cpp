/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2017, Bielefeld University
 *  Copyright (c) 2017, Hamburg University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Bielefeld University nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Authors: Michael Goerner, Luca Lach, Robert Haschke */

#include <moveit/task_constructor/stages/current_state.h>
#include <moveit/task_constructor/storage.h>
#include <moveit_msgs/GetPlanningScene.h>
#include <moveit_msgs/PlanningSceneComponents.h>
#include <moveit/planning_scene/planning_scene.h>
#include <moveit/robot_model_loader/robot_model_loader.h>
#include <ros/ros.h>


namespace moveit { namespace task_constructor { namespace stages {

CurrentState::CurrentState(const std::string &name)
   : Generator(name)
{
	auto &p = properties();
	p.declare<ros::Duration>("timeout", ros::Duration(-1), "max time to wait for get_planning_scene service");
}

void CurrentState::init(const planning_scene::PlanningSceneConstPtr &scene)
{
	Generator::init(scene);
	scene_ = scene->diff();
}

bool CurrentState::canCompute() const{
	return !ran_;
}

bool CurrentState::compute(){
	auto rml = std::make_shared<robot_model_loader::RobotModelLoader>();
	scene_ = std::make_shared<planning_scene::PlanningScene>(rml->getModel());

	ros::NodeHandle h;
	ros::ServiceClient client = h.serviceClient<moveit_msgs::GetPlanningScene>("get_planning_scene");

	ros::Duration timeout = properties().get<ros::Duration>("timeout");
	if (client.waitForExistence(timeout)) {
		moveit_msgs::GetPlanningScene::Request req;
		moveit_msgs::GetPlanningScene::Response res;

		req.components.components =
		      moveit_msgs::PlanningSceneComponents::SCENE_SETTINGS
		      | moveit_msgs::PlanningSceneComponents::ROBOT_STATE
		      | moveit_msgs::PlanningSceneComponents::ROBOT_STATE_ATTACHED_OBJECTS
		      | moveit_msgs::PlanningSceneComponents::WORLD_OBJECT_NAMES
		      | moveit_msgs::PlanningSceneComponents::WORLD_OBJECT_GEOMETRY
		      | moveit_msgs::PlanningSceneComponents::OCTOMAP
		      | moveit_msgs::PlanningSceneComponents::TRANSFORMS
		      | moveit_msgs::PlanningSceneComponents::ALLOWED_COLLISION_MATRIX
		      | moveit_msgs::PlanningSceneComponents::LINK_PADDING_AND_SCALING
		      | moveit_msgs::PlanningSceneComponents::OBJECT_COLORS;

		if (client.call(req, res)) {
			scene_->setPlanningSceneMsg(res.scene);
			spawn(InterfaceState(scene_), 0.0);
			ran_= true;
			return true;
		}
	}
	ROS_WARN("failed to acquire current PlanningScene");
	return false;
}

} } }

/// register plugin
#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(moveit::task_constructor::stages::CurrentState, moveit::task_constructor::Stage)
