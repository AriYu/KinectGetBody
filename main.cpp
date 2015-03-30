#include "message.h"
#include "transport.h"


#include <Windows.h>
#include <Kinect.h>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>

using namespace boost::asio;

template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL){
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

int main(int argc, char* argv[])
{
	try{
		cv::setUseOptimized(true);

		// Sensor
		IKinectSensor* pSensor;
		HRESULT hResult = S_OK;
		hResult = GetDefaultKinectSensor(&pSensor);
		if (FAILED(hResult)){
			std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
			return -1;
		}

		hResult = pSensor->Open();
		if (FAILED(hResult)){
			std::cerr << "Error : IKinectSensor::Open()" << std::endl;
			return -1;
		}

		// Source
		IColorFrameSource* pColorSource;
		hResult = pSensor->get_ColorFrameSource(&pColorSource);
		if (FAILED(hResult)){
			std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
			return -1;
		}

		IBodyFrameSource* pBodySource;
		hResult = pSensor->get_BodyFrameSource(&pBodySource);
		if (FAILED(hResult)){
			std::cerr << "Error : IKinectSensor::get_BodyFrameSource()" << std::endl;
			return -1;
		}

		// Reader
		IColorFrameReader* pColorReader;
		hResult = pColorSource->OpenReader(&pColorReader);
		if (FAILED(hResult)){
			std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
			return -1;
		}

		IBodyFrameReader* pBodyReader;
		hResult = pBodySource->OpenReader(&pBodyReader);
		if (FAILED(hResult)){
			std::cerr << "Error : IBodyFrameSource::OpenReader()" << std::endl;
			return -1;
		}

		// Description
		IFrameDescription* pDescription;
		hResult = pColorSource->get_FrameDescription(&pDescription);
		if (FAILED(hResult)){
			std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
			return -1;
		}

		int width = 0;
		int height = 0;
		pDescription->get_Width(&width); // 1920
		pDescription->get_Height(&height); // 1080
		unsigned int bufferSize = width * height * 4 * sizeof(unsigned char);

		cv::Mat bufferMat(height, width, CV_8UC4);
		cv::Mat bodyMat(height / 2, width / 2, CV_8UC4);
		cv::namedWindow("Body");

		// Color Table
		cv::Vec3b color[BODY_COUNT];
		color[0] = cv::Vec3b(255, 0, 0);
		color[1] = cv::Vec3b(0, 255, 0);
		color[2] = cv::Vec3b(0, 0, 255);
		color[3] = cv::Vec3b(255, 255, 0);
		color[4] = cv::Vec3b(255, 0, 255);
		color[5] = cv::Vec3b(0, 255, 255);

		// Coordinate Mapper
		ICoordinateMapper* pCoordinateMapper;
		hResult = pSensor->get_CoordinateMapper(&pCoordinateMapper);
		if (FAILED(hResult)){
			std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
			return -1;
		}

		// Message
		message myMessage;

		// Transport
		boost::asio::io_service io_service;
		std::string ip_address("192.168.0.155");
		std::string port_number("18080");
		transport transporter(io_service, ip_address, port_number);

		while (1){
			

			// Frame
			IColorFrame* pColorFrame = nullptr;
			hResult = pColorReader->AcquireLatestFrame(&pColorFrame);
			if (SUCCEEDED(hResult)){
				hResult = pColorFrame->CopyConvertedFrameDataToArray(bufferSize, reinterpret_cast<BYTE*>(bufferMat.data), ColorImageFormat::ColorImageFormat_Bgra);
				if (SUCCEEDED(hResult)){
					cv::resize(bufferMat, bodyMat, cv::Size(), 0.5, 0.5);
				}
			}
			//SafeRelease( pColorFrame );

			IBodyFrame* pBodyFrame = nullptr;
			hResult = pBodyReader->AcquireLatestFrame(&pBodyFrame);
			if (SUCCEEDED(hResult)){
				IBody* pBody[BODY_COUNT] = { 0 };
				hResult = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, pBody);
				if (SUCCEEDED(hResult)){
					for (int count = 0; count < BODY_COUNT; count++){
						BOOLEAN bTracked = false;
						myMessage.bodies_[count].isTracked_ = false;
						hResult = pBody[count]->get_IsTracked(&bTracked);
						if (SUCCEEDED(hResult) && bTracked){
							myMessage.bodies_[count].isTracked_ = true;
							Joint joint[JointType::JointType_Count];
							hResult = pBody[count]->GetJoints(JointType::JointType_Count, joint);
							if (SUCCEEDED(hResult)){
								// Left Hand State
								HandState leftHandState = HandState::HandState_Unknown;
								myMessage.bodies_[count].left_hand_state_ = body::HandState::HandState_Unknown;
								hResult = pBody[count]->get_HandLeftState(&leftHandState);
								if (SUCCEEDED(hResult)){
									ColorSpacePoint colorSpacePoint = { 0 };
									hResult = pCoordinateMapper->MapCameraPointToColorSpace(joint[JointType::JointType_HandLeft].Position, &colorSpacePoint);
									if (SUCCEEDED(hResult)){
										int x = static_cast<int>(colorSpacePoint.X);
										int y = static_cast<int>(colorSpacePoint.Y);
										if ((x >= 0) && (x < width) && (y >= 0) && (y < height)){
											if (leftHandState == HandState::HandState_Open){
												cv::circle(bufferMat, cv::Point(x, y), 75, cv::Scalar(0, 128, 0), 5, CV_AA);
												myMessage.bodies_[count].left_hand_state_ = body::HandState::HandState_Open;
											}
											else if (leftHandState == HandState::HandState_Closed){
												cv::circle(bufferMat, cv::Point(x, y), 75, cv::Scalar(0, 0, 128), 5, CV_AA);
												myMessage.bodies_[count].left_hand_state_ = body::HandState::HandState_Closed;
											}
											else if (leftHandState == HandState::HandState_Lasso){
												cv::circle(bufferMat, cv::Point(x, y), 75, cv::Scalar(128, 128, 0), 5, CV_AA);
												myMessage.bodies_[count].left_hand_state_ = body::HandState::HandState_Lasso;
											}
										}
									}
								}

								// Right Hand State
								HandState rightHandState = HandState::HandState_Unknown;
								myMessage.bodies_[count].right_hand_state_ = body::HandState::HandState_Unknown;
								hResult = pBody[count]->get_HandRightState(&rightHandState);
								if (SUCCEEDED(hResult)){
									ColorSpacePoint colorSpacePoint = { 0 };
									hResult = pCoordinateMapper->MapCameraPointToColorSpace(joint[JointType::JointType_HandRight].Position, &colorSpacePoint);
									if (SUCCEEDED(hResult)){
										int x = static_cast<int>(colorSpacePoint.X);
										int y = static_cast<int>(colorSpacePoint.Y);
										if ((x >= 0) && (x < width) && (y >= 0) && (y < height)){
											if (rightHandState == HandState::HandState_Open){
												cv::circle(bufferMat, cv::Point(x, y), 75, cv::Scalar(0, 128, 0), 5, CV_AA);
												myMessage.bodies_[count].right_hand_state_ = body::HandState::HandState_Open;
											}
											else if (rightHandState == HandState::HandState_Closed){
												cv::circle(bufferMat, cv::Point(x, y), 75, cv::Scalar(0, 0, 128), 5, CV_AA);
												myMessage.bodies_[count].right_hand_state_ = body::HandState::HandState_Closed;
											}
											else if (rightHandState == HandState::HandState_Lasso){
												cv::circle(bufferMat, cv::Point(x, y), 75, cv::Scalar(128, 128, 0), 5, CV_AA);
												myMessage.bodies_[count].right_hand_state_ = body::HandState::HandState_Lasso;
											}
										}
									}
								}

								// Joint
								for (int type = 0; type < JointType::JointType_Count; type++){
									ColorSpacePoint colorSpacePoint = { 0 };
									pCoordinateMapper->MapCameraPointToColorSpace(joint[type].Position, &colorSpacePoint);
									int x = static_cast<int>(colorSpacePoint.X);
									int y = static_cast<int>(colorSpacePoint.Y);
									myMessage.bodies_[count].positions_[type].x_ = colorSpacePoint.X;
									myMessage.bodies_[count].positions_[type].y_ = colorSpacePoint.Y;
									myMessage.bodies_[count].positions_[type].z_ = joint[count].Position.Z;
									if ((x >= 0) && (x < width) && (y >= 0) && (y < height)){
										cv::circle(bufferMat, cv::Point(x, y), 5, static_cast<cv::Scalar>(color[count]), -1, CV_AA);
									}
								}
							}

							// Lean
							PointF amount;
							hResult = pBody[count]->get_Lean(&amount);
							if (SUCCEEDED(hResult)){
								//std::cout << "amount : " << amount.X << ", " << amount.Y << std::endl;
							}
						}
					}
					cv::resize(bufferMat, bodyMat, cv::Size(), 0.5, 0.5);
				}
				for (int count = 0; count < BODY_COUNT; count++){
					SafeRelease(pBody[count]);
				}
			}
			//SafeRelease( pBodyFrame );

			SafeRelease(pColorFrame);
			SafeRelease(pBodyFrame);

			cv::imshow("Body", bodyMat);
			size_t send_size = transporter.send(myMessage);
			std::cout << "send ... " << send_size << "byte" << std::endl;
			if (cv::waitKey(10) == VK_ESCAPE){
				break;
			}
		}

		SafeRelease(pColorSource);
		SafeRelease(pBodySource);
		SafeRelease(pColorReader);
		SafeRelease(pBodyReader);
		SafeRelease(pDescription);
		SafeRelease(pCoordinateMapper);
		if (pSensor){
			pSensor->Close();
		}
		SafeRelease(pSensor);
		cv::destroyAllWindows();

		return 0;
	}
	catch (std::exception& e){
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
}

