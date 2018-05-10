#include "dvrk_arm/Arm.h"
DVRK_Arm::DVRK_Arm(const std::string &arm_name){
    m_originFramePtr.reset(new Frame);
    m_eeFramePtr.reset(new Frame);
    m_afxdTipFramePtr.reset(new Frame);
    m_freeFramePtr.reset(new Frame);


    m_frameptrVec.push_back(m_originFramePtr);
    m_frameptrVec.push_back(m_eeFramePtr);
    m_frameptrVec.push_back(m_afxdTipFramePtr);

    m_bridge.reset(new DVRK_Bridge(arm_name));
    m_bridge->poseFcnHandle.assign_fcn(&DVRK_Arm::pose_fcn_cb, this);
    m_bridge->jointFcnHandle.assign_fcn(&DVRK_Arm::joint_state_fcn_cb, this);
    m_bridge->wrenchFcnHandle.assign_fcn(&DVRK_Arm::wrench_fcn_cb, this);
    m_bridge->gripperFcnHandle.assign_fcn(&DVRK_Arm::gripper_state_fcn_cb, this);
    m_counter = 0;
}

void DVRK_Arm::init(){
}

void DVRK_Arm::pose_fcn_cb(const geometry_msgs::PoseStamped &pose){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    m_freeFramePtr->pos.setX(pose.pose.position.x);
    m_freeFramePtr->pos.setY(pose.pose.position.y);
    m_freeFramePtr->pos.setZ(pose.pose.position.z);
    tf::quaternionMsgToTF(pose.pose.orientation, m_freeFramePtr->rot_quat);

    m_freeFramePtr->trans.setOrigin(m_freeFramePtr->pos);
    m_freeFramePtr->trans.setRotation(m_freeFramePtr->rot_quat);
    m_freeFramePtr->trans = m_originFramePtr->trans.inverse() * m_freeFramePtr->trans * m_afxdTipFramePtr->trans;
    m_eeFramePtr->trans = m_freeFramePtr->trans;
    handle_frames();
}

void DVRK_Arm::gripper_state_fcn_cb(const sensor_msgs::JointState &state){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    m_gripper_angle = state.position[0];
}

void DVRK_Arm::joint_state_fcn_cb(const sensor_msgs::JointState &jnt){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (m_jointPos.size() != jnt.position.size()){
        m_jointPos.resize(jnt.position.size());
    }
    if (m_jointVel.size() != jnt.velocity.size()){
        m_jointVel.resize(jnt.velocity.size());
    }
    if (m_jointEffort.size() != jnt.effort.size()){
        m_jointEffort.resize(jnt.effort.size());
    }
    m_jointPos = jnt.position;
    m_jointVel = jnt.velocity;
    m_jointEffort = jnt.effort;
}

void DVRK_Arm::wrench_fcn_cb(const geometry_msgs::WrenchStamped &wrench){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    tf::vector3MsgToTF(wrench.wrench.force, m_wrenchForce);
    tf::vector3MsgToTF(wrench.wrench.torque, m_wrenchMoment);
    m_wrenchForce = m_eeFramePtr->rot_mat.inverse() * m_wrenchForce;
    m_wrenchMoment = m_eeFramePtr->rot_mat.inverse() * m_wrenchMoment;
}

void DVRK_Arm::set_origin_frame(const tf::Vector3 &pos, const tf::Matrix3x3 &tf_mat){
    tf_mat.getRotation(m_originFramePtr->rot_quat);
    set_origin_frame(pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::set_origin_frame(const tf::Vector3 &pos, const tf::Quaternion &tf_quat){
    m_originFramePtr->trans.setOrigin(pos);
    m_originFramePtr->trans.setRotation(tf_quat);
}

void DVRK_Arm::set_origin_frame(const tf::Transform &trans){
    m_originFramePtr->trans = trans;
}

void DVRK_Arm::set_origin_frame_pos(const double &x, const double &y, const double &z){
    m_originFramePtr->pos.setX(x);
    m_originFramePtr->pos.setY(y);
    m_originFramePtr->pos.setZ(z);

    set_origin_frame(m_originFramePtr->pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::set_origin_frame_pos(const geometry_msgs::Point &pos){
    m_originFramePtr->pos.setX(pos.x);
    m_originFramePtr->pos.setY(pos.y);
    m_originFramePtr->pos.setZ(pos.z);

    set_origin_frame(m_originFramePtr->pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::set_origin_frame_pos(const tf::Vector3 &pos){
    m_originFramePtr->pos = pos;

    set_origin_frame(m_originFramePtr->pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::set_origin_frame_rot(const double &roll, const double &pitch, const double &yaw){
    m_originFramePtr->rot_quat.setRPY(roll, pitch, yaw);

    set_origin_frame(m_originFramePtr->pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::set_origin_frame_rot(const tf::Quaternion &tf_quat){
    m_originFramePtr->rot_quat = tf_quat;

    set_origin_frame(m_originFramePtr->pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::set_origin_frame_rot(const geometry_msgs::Quaternion &gm_quat){
    tf::quaternionMsgToTF(gm_quat, m_originFramePtr->rot_quat);

    set_origin_frame(m_originFramePtr->pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::set_origin_frame_rot(const tf::Matrix3x3 &mat){
    mat.getRotation(m_originFramePtr->rot_quat);

    set_origin_frame(m_originFramePtr->pos, m_originFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame_pos(const double &x, const double &y, const double &z){
    m_afxdTipFramePtr->pos.setX(x);
    m_afxdTipFramePtr->pos.setY(y);
    m_afxdTipFramePtr->pos.setZ(z);
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame_pos(const geometry_msgs::Point &pos){
    m_afxdTipFramePtr->pos.setX(pos.x);
    m_afxdTipFramePtr->pos.setY(pos.y);
    m_afxdTipFramePtr->pos.setZ(pos.z);
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame_pos(const tf::Vector3 &pos){
    m_afxdTipFramePtr->pos = pos;
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame_rot(const double &roll, const double &pitch, const double &yaw){
    m_afxdTipFramePtr->rot_quat.setRPY(roll, pitch, yaw);
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame_rot(const double &quat_x, const double &quat_y, const double &quat_z, const double &quat_w){
    m_afxdTipFramePtr->rot_quat.setX(quat_x);
    m_afxdTipFramePtr->rot_quat.setY(quat_y);
    m_afxdTipFramePtr->rot_quat.setZ(quat_z);
    m_afxdTipFramePtr->rot_quat.setW(quat_w);
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame_rot(const geometry_msgs::Quaternion &gm_quat){
    tf::quaternionMsgToTF(gm_quat, m_afxdTipFramePtr->rot_quat);
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame_rot(const tf::Quaternion &tf_quat){
    m_afxdTipFramePtr->rot_quat = tf_quat;
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame(const tf::Vector3 &pos, const tf::Quaternion &tf_quat){
    m_afxdTipFramePtr->trans.setOrigin(pos);
    m_afxdTipFramePtr->trans.setRotation(tf_quat);
}

void DVRK_Arm::affix_tip_frame(const tf::Vector3 &pos, const tf::Matrix3x3 &tf_mat){
    m_afxdTipFramePtr->pos = pos;
    tf_mat.getRotation(m_afxdTipFramePtr->rot_quat);
    affix_tip_frame(m_afxdTipFramePtr->pos, m_afxdTipFramePtr->rot_quat);
}

void DVRK_Arm::affix_tip_frame(const tf::Transform &trans){
    m_afxdTipFramePtr->trans = trans;
}

void DVRK_Arm::measured_cp_pos(double &x, double &y, double &z){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    x = m_eeFramePtr->trans.getOrigin().getX();
    y = m_eeFramePtr->trans.getOrigin().getY();
    z = m_eeFramePtr->trans.getOrigin().getZ();
}

void DVRK_Arm::measured_cp_pos(tf::Vector3 &pos){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    pos = m_eeFramePtr->trans.getOrigin();
}

void DVRK_Arm::measured_cp_pos(geometry_msgs::Point &pos){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    tf::pointTFToMsg(m_eeFramePtr->trans.getOrigin(), pos);
}

void DVRK_Arm::measured_cp_ori(double &roll, double &pitch, double &yaw){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    tf::Matrix3x3(m_eeFramePtr->trans.getRotation()).getRPY(roll, pitch, yaw);
}

void DVRK_Arm::measured_cp_ori(double &x, double &y, double &z, double &w){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    x = m_eeFramePtr->trans.getRotation().getX();
    y = m_eeFramePtr->trans.getRotation().getY();
    z = m_eeFramePtr->trans.getRotation().getZ();
    w = m_eeFramePtr->trans.getRotation().getW();
}

void DVRK_Arm::measured_cp_ori(tf::Quaternion &tf_quat){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    tf_quat = m_eeFramePtr->trans.getRotation();
    tf_quat.normalize();
}

void DVRK_Arm::measured_cp_ori(geometry_msgs::Quaternion &gm_quat){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    tf::quaternionTFToMsg(m_eeFramePtr->trans.getRotation(), gm_quat);
}

void DVRK_Arm::measured_cp_ori(tf::Matrix3x3 &mat){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    mat.setRotation(m_eeFramePtr->trans.getRotation());
}

void DVRK_Arm::measured_cp(geometry_msgs::Pose &pose){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    pose.position.x = m_eeFramePtr->trans.getOrigin().getX();
    pose.position.y = m_eeFramePtr->trans.getOrigin().getY();
    pose.position.z = m_eeFramePtr->trans.getOrigin().getZ();

    tf::quaternionTFToMsg(m_eeFramePtr->trans.getRotation(), pose.orientation);
}

void DVRK_Arm::measured_cp(tf::Transform &trans){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    trans = m_eeFramePtr->trans;
    trans.setRotation(trans.getRotation().normalized());
}

void DVRK_Arm::measured_cf_force(double &fx, double &fy, double &fz) {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    fx = m_wrenchForce.getX();
    fy = m_wrenchForce.getY();
    fz = m_wrenchForce.getZ();
}

void DVRK_Arm::measured_cf_moment(double &nx, double &ny, double &nz){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    nx = m_wrenchMoment.getX();
    ny = m_wrenchMoment.getY();
    nz = m_wrenchMoment.getZ();
}

void DVRK_Arm::measured_cf(double &fx, double &fy, double &fz, double &nx, double &ny, double &nz) {
    measured_cf_force(fx, fy, fz);
    measured_cf_moment(nx, ny, nz);
}

void DVRK_Arm::measured_jp(std::vector<double> &jnt_pos) {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (jnt_pos.size() != m_jointPos.size()){
        jnt_pos.resize(m_jointPos.size());
    }
    jnt_pos = m_jointPos;
}

void DVRK_Arm::measured_jv(std::vector<double> &jnt_vel) {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (jnt_vel.size() != m_jointVel.size()){
        jnt_vel.resize(m_jointVel.size());
    }
    jnt_vel = m_jointPos;
}

void DVRK_Arm::measured_jf(std::vector<double> &jnt_effort) {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    if (jnt_effort.size() != m_jointEffort.size()){
        jnt_effort.resize(m_jointEffort.size());
    }
    jnt_effort = m_jointPos;
}

void DVRK_Arm::measured_gripper_angle(double &pos){
    boost::lock_guard<boost::mutex> lock(m_mutex);
    pos = m_gripper_angle;
}

bool DVRK_Arm::move_cp_pos(const double &x, const double &y, const double &z){
    m_eeCmd.trans.setOrigin(tf::Vector3(x,y,z));
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp_pos(const geometry_msgs::Point &pos){
    m_eeCmd.trans.setOrigin(tf::Vector3(pos.x, pos.y, pos.z));
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp_pos(const tf::Vector3 &pos){
    m_eeCmd.trans.setOrigin(pos);
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp_ori(const double &roll, const double &pitch, const double &yaw){
    m_eeCmd.rot_quat.setRPY(roll, pitch, yaw);
    m_eeCmd.trans.setRotation(m_eeCmd.rot_quat);
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp_ori(const double &x, const double &y, const double &z, const double &w){
    m_eeCmd.trans.setRotation(tf::Quaternion(x,y,z,w));
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp_ori(const tf::Quaternion &tf_quat){
    m_eeCmd.trans.setRotation(tf_quat);
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp_ori(const geometry_msgs::Quaternion &gm_quat){
    m_eeCmd.trans.setRotation(tf::Quaternion(gm_quat.x, gm_quat.y, gm_quat.z, gm_quat.w));
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp_ori(const tf::Matrix3x3 &mat){
    mat.getRotation(m_eeCmd.rot_quat);
    m_eeCmd.trans.setRotation(m_eeCmd.rot_quat);
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp(geometry_msgs::PoseStamped &pose){
    m_eeCmd.trans.setOrigin(tf::Vector3(pose.pose.position.x,
                           pose.pose.position.y,
                           pose.pose.position.z));

    m_eeCmd.trans.setRotation(tf::Quaternion(pose.pose.orientation.x,
                             pose.pose.orientation.y,
                             pose.pose.orientation.z,
                             pose.pose.orientation.w));
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::move_cp(tf::Transform &trans){
    m_eeCmd.trans = trans;
    move_arm_cartesian(m_eeCmd.trans);
}

bool DVRK_Arm::is_gripper_pressed(){
    return m_bridge->_gripper_closed;
}

bool DVRK_Arm::is_clutch_pressed(){
    return m_bridge->_clutch_pressed;
}

bool DVRK_Arm::is_coag_pressed(){
    return m_bridge->_coag_pressed;
}

void DVRK_Arm::set_mode(const std::string &state, bool lock_wrench_ori){
    m_bridge->set_cur_mode(state, lock_wrench_ori);
}

void DVRK_Arm::move_arm_cartesian(tf::Transform trans){
    geometry_msgs::PoseStamped cmd_pose;
    trans = m_originFramePtr->trans * trans * m_afxdTipFramePtr->trans.inverse();
    cmd_pose.pose.position.x = trans.getOrigin().getX();
    cmd_pose.pose.position.y = trans.getOrigin().getY();
    cmd_pose.pose.position.z = trans.getOrigin().getZ();
    tf::quaternionTFToMsg(trans.getRotation().normalized(), cmd_pose.pose.orientation);

    m_bridge->set_cur_pose(cmd_pose);
}

bool DVRK_Arm::set_force(const double &fx, const double &fy, const double &fz){
    m_eeCmd.force.setX(fx);
    m_eeCmd.force.setY(fy);
    m_eeCmd.force.setZ(fz);

    set_arm_wrench(m_eeCmd.force, m_eeCmd.moment);
}

bool DVRK_Arm::set_moment(const double &nx, const double &ny, const double &nz){
    m_eeCmd.moment.setX(nx);
    m_eeCmd.moment.setY(ny);
    m_eeCmd.moment.setZ(nz);

    set_arm_wrench(m_eeCmd.force, m_eeCmd.moment);
}

bool DVRK_Arm::set_wrench(const double &fx,const double &fy,const double &fz,const double &nx,const double &ny,const double &nz){
    m_eeCmd.force.setX(fx);
    m_eeCmd.force.setY(fy);
    m_eeCmd.force.setZ(fz);
    m_eeCmd.moment.setX(nx);
    m_eeCmd.moment.setY(ny);
    m_eeCmd.moment.setZ(nz);
    set_arm_wrench(m_eeCmd.force, m_eeCmd.moment);
}

void DVRK_Arm::set_arm_wrench(tf::Vector3 &force, tf::Vector3 &moment){
    if(m_bridge->_clutch_pressed == true){
        force.setZero();
        moment.setZero();
    }
    m_originFramePtr->rot_mat.setRotation(m_originFramePtr->trans.getRotation());
    geometry_msgs::Wrench cmd_wrench;
    tf::vector3TFToMsg(m_originFramePtr->rot_mat * force, cmd_wrench.force);
    tf::vector3TFToMsg(m_originFramePtr->rot_mat * moment, cmd_wrench.torque);
    m_bridge->set_cur_wrench(cmd_wrench);
}

void DVRK_Arm::handle_frames(){
    for(m_frameIter = m_frameptrVec.begin(); m_frameIter !=m_frameptrVec.end(); m_frameIter++){
        double x,y,z;
        x = (*m_frameIter)->trans.getOrigin().getX();
        y = (*m_frameIter)->trans.getOrigin().getY();
        z = (*m_frameIter)->trans.getOrigin().getZ();

        double qx, qy, qz, qw;
        qx = (*m_frameIter)->trans.getRotation().getX();
        qy = (*m_frameIter)->trans.getRotation().getY();
        qz = (*m_frameIter)->trans.getRotation().getZ();
        qw = (*m_frameIter)->trans.getRotation().getW();
        if (std::isnan(x) || std::isnan(y) ||  std::isnan(z)){
            (*m_frameIter)->trans.setOrigin(tf::Vector3(0,0,0));
            std::cerr<< "Origin of frame is NAN, setting origin to (0,0,0)" << std::endl;
        }
        if (std::isnan(qx) || std::isnan(qy) ||  std::isnan(qz) || std::isnan(qw)){
            (*m_frameIter)->trans.setRotation(tf::Quaternion().getIdentity());
            std::cerr<< "Rotation of frame is NAN, setting rotation to (0,0,0)" << std::endl;
        }
        //Normalize the rotation quaternion;
        (*m_frameIter)->trans.getRotation() = (*m_frameIter)->trans.getRotation().normalized();
        //Setting the pos, quat and rot_mat members of the struct so all of them have the same data as trans
        (*m_frameIter)->pos = (*m_frameIter)->trans.getOrigin();
        (*m_frameIter)->rot_quat = (*m_frameIter)->trans.getRotation();
        (*m_frameIter)->rot_mat.setRotation((*m_frameIter)->rot_quat);
    }
    m_counter++;
    if (m_counter % 15 == 0){
//    frame_broadcaster.sendTransform(tf::StampedTransform(m_originFramePtr->trans, ros::Time::now(), "world", "arm_origin"));
//    frame_broadcaster.sendTransform(tf::StampedTransform(m_eeFramePtr->trans, ros::Time::now(), "arm_origin", "ee"));
    m_counter = 0;
    }
}

bool DVRK_Arm::close(){
    m_bridge->shutDown();
    return true;
}

DVRK_Arm::~DVRK_Arm(){
    std::cerr << "CLOSING DVRK_ARM" << std::endl;
}


extern "C"{
std::vector<std::string> get_active_arms(){
    std::vector<std::string> active_arm_names;
    DVRK_Bridge::get_arms_from_rostopics(active_arm_names);
    return active_arm_names;
}
boost::shared_ptr<DVRK_Arm> create(std::string arm_name){
    return boost::shared_ptr<DVRK_Arm>(new DVRK_Arm(arm_name));
}
void destroy(boost::shared_ptr<DVRK_Arm> arm_obj){
    arm_obj.reset();
}
}
