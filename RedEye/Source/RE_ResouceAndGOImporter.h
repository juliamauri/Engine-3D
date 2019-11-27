#ifndef __RE_RESOURCEANDGOIMPORTER_H__
#define __RE_RESOURCEANDGOIMPORTER_H__

class RE_GameObject;
class JSONNode;

class RE_ResouceAndGOImporter
{
public:
	static void JsonSerialize(JSONNode* node, RE_GameObject* toSerialize);
	static char* BinarySerialize(RE_GameObject* toSerialize, unsigned int* bufferSize);

	static RE_GameObject* JsonDeserialize(JSONNode* node);
	static RE_GameObject* BinaryDeserialize(char*& cursor);
};

#endif // !__RE_RESOURCEANDGOIMPORTER_H__