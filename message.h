#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <iostream>
#include <vector>

#include <boost/serialization/vector.hpp>

class position
{
public:
	position() :x_(0), y_(0), z_(0){}
	position(double x, double y, double z){
		x_ = x;
		y_ = y;
		z_ = z;
	}

	double x_;
	double y_;
	double z_;

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & x_;
		ar & y_;
		ar & z_;
	}
};

class body
{
public:
	enum { joint_count = 25 };
	enum HandState
	{
		HandState_Unknown    = 0,
		HandState_NotTracked = 1,
		HandState_Open       = 2,
		HandState_Closed     = 3,
		HandState_Lasso      = 4
	};
	body(){
		positions_.resize(body::joint_count + 1);
		right_hand_state_ = body::HandState::HandState_Unknown;
		left_hand_state_  = body::HandState::HandState_Unknown;
		isTracked_ = false;
	}
	std::vector< position > positions_;
	HandState right_hand_state_;
	HandState left_hand_state_;
	bool isTracked_;
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		size_t size = positions_.size();
		for (size_t i = 0; i < size; i++)
		{
			ar & positions_[i];
		}
		ar & right_hand_state_;
		ar & left_hand_state_;
		ar & isTracked_;
	}
};

class message
{
public:
	enum { width = 1920 }; // color image width
	enum { height = 1080 }; // color image height
	enum { body_count = 6 };
	message()
	{
		bodies_.resize( message::body_count + 1);		
	}

	std::vector< body > bodies_;

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		size_t size = bodies_.size();
		for (size_t i = 0; i < size; i++)
		{
			ar & bodies_[i];
		}

	}
};


class beta_body
{
public:
	beta_body(){}
	//std::vector< position > positions_;
	position positions_[25];
	bool isTracked_;
	int right_hand_state_;
	int left_hand_state_;
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & positions_;
		//ar & boost::serialization::make_array(&positions_, sizeof(positions_));
		ar & isTracked_;
		ar & right_hand_state_;
		ar & left_hand_state_;
	}
};
class beta_message
{
public:
	beta_message(){}
	//std::vector< beta_body > bodies_;
	beta_body bodies_[6];
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 25; j++)
			{
				/*std::cout << "p[" << i << "][" << j << "]x:" << bodies_[i].positions_[j].x_ << std::endl;
				std::cout << "p[" << i << "][" << j << "]y:" << bodies_[i].positions_[j].y_ << std::endl;
				std::cout << "p[" << i << "][" << j << "]z:" << bodies_[i].positions_[j].z_ << std::endl;*/
				ar & bodies_[i].positions_[j].x_;
				ar & bodies_[i].positions_[j].y_;
				ar & bodies_[i].positions_[j].z_;
				//std::cout << "a[" << i << "][" << j << "]x:" << bodies_[i].positions_[j].x_ << std::endl;
				//std::cout << "a[" << i << "][" << j << "]y:" << bodies_[i].positions_[j].y_ << std::endl;
				//std::cout << "a[" << i << "][" << j << "]z:" << bodies_[i].positions_[j].z_ << std::endl;
				/*ar & bodies_[i].positions_[j].x_;
				ar & bodies_[i].positions_[j].y_;
				ar & bodies_[i].positions_[j].z_;*/
			}
			ar & bodies_[i].isTracked_;
			ar & bodies_[i].right_hand_state_;
			ar & bodies_[i].left_hand_state_;
		}
		//		ar & bodies_;
		//ar & boost::serialization::make_array(&bodies_, sizeof(bodies_));
	}
};

#endif