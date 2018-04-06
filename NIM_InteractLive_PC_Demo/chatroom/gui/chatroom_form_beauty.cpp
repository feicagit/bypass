#include"chatroom_form.h"
namespace nim_chatroom
{

	void ChatroomForm::BeautyOperate()
	{
		if (!is_open_beauty_)
		{
			is_open_beauty_ = true;
			nim::VChat::SetCustomData(false, true);
			video_frame_mng_.SetBeautyOpt(true);
		}
		else
		{
			is_open_beauty_ = false;

			nim::VChat::SetCustomData(false, false);
			video_frame_mng_.SetBeautyOpt(false);
		}
	}

}