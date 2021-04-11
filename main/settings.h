/*
 * Setting.h
 *
 *  Created on: Mar 17, 2021
 *      Author: Bas
 */

#ifndef MAIN_SETTINGS_H_
#define MAIN_SETTINGS_H_

#include <vector>
#include <stdint.h>
#include "nvs_flash.h"
#include "../lib/freertos_cpp/semaphore.h"


class BaseSettings;


class ISaveable
{
protected:
	virtual ~ISaveable(){}
	virtual void Save(nvs_handle_t my_handle) = 0;
	virtual void Load(nvs_handle_t my_handle) = 0;
	friend BaseSettings;
};


template<typename T>
class BaseSetting : public ISaveable
{
protected:
	FreeRTOS::Mutex mutex;
	const char *name;
	bool changed = false;
	T value;

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, T val) = 0;
	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, T *val) = 0;

	virtual void Save(nvs_handle_t my_handle) override
	{
		if(changed)
		{
			if(mutex.Take())
			{
				esp_err_t err = DoSave(my_handle, name, value);
				if(err == ESP_OK)
					changed = false;
				else
					ESP_LOGE("Setting", "Error while saving '%s', '%s'", name, esp_err_to_name(err));
				mutex.Give();
			}
		}
	}

	virtual void Load(nvs_handle_t my_handle) override
	{
		if(mutex.Take())
		{
			esp_err_t err = DoLoad(my_handle, name, &value);
			if(err == ESP_OK)
				changed = false;
			else
				ESP_LOGE("Setting", "Error while loading '%s', '%s'", name, esp_err_to_name(err));
			mutex.Give();
		}
	}

public:

	T Get()
	{
		return value;
	}

	void Set(T val)
	{
		if(mutex.Take())
		{
			changed = true; //TODO: Could be optimized
			value = val;
			mutex.Give();
		}
	}

	virtual ~BaseSetting(){}

	BaseSetting(const char *name, std::vector<ISaveable *> *list)
	{
		this->name = name;
		if(list != NULL)
			list->push_back(this);
	}

	BaseSetting(const char *name, T defVal, std::vector<ISaveable*> *list)
	{
		this->name = name;
		Set(defVal);
		if(list != NULL)
			list->push_back(this);
	}
};


template<typename T>
class Setting : public BaseSetting<T>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<T>(name, list){}
	Setting(const char *name, T defVal, std::vector<ISaveable*> *list) : BaseSetting<T>(name, defVal, list){}

};


class BaseSettings
{
	bool isInitialized = false;
	const char *nSpace;

	inline void InitIfNecessary()
	{
		if(isInitialized == false)
		{
			esp_err_t err = nvs_flash_init();
			if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
			{
				ESP_ERROR_CHECK(nvs_flash_erase());
				err = nvs_flash_init();
			}
			ESP_ERROR_CHECK( err );
			isInitialized = true;
		}
	}

protected:
	std::vector<ISaveable*> settingsList;

public:

	void Save()
	{
		esp_err_t err;
		nvs_handle_t my_handle;
		InitIfNecessary();
		err = nvs_open(nSpace, NVS_READWRITE, &my_handle);
		if (err != ESP_OK)
		{
			ESP_LOGE("Setting", "Error while opening handle '%s'", esp_err_to_name(err));
		}
		else
		{
			for(int i=0; i<settingsList.size(); i++)
				settingsList[i]->Save(my_handle);

			err = nvs_commit(my_handle);
			if (err != ESP_OK)
			{
				ESP_LOGE("Setting", "Error while commiting changes '%s'", esp_err_to_name(err));
			}
			nvs_close(my_handle);
		}
	}

	void Load()
	{
		esp_err_t err;
		nvs_handle_t my_handle;
		InitIfNecessary();
		err = nvs_open(nSpace, NVS_READWRITE, &my_handle);
		if (err != ESP_OK)
		{
			ESP_LOGE("Setting", "Error while opening handle '%s'", esp_err_to_name(err));
		}
		else
		{
			for(int i=0; i<settingsList.size(); i++)
				settingsList[i]->Load(my_handle);
			nvs_close(my_handle);
		}
	}

	BaseSettings(const char *nSpace)
	{
		this->nSpace = nSpace;
	}

};

template<>
class Setting<uint8_t> : public BaseSetting<uint8_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<uint8_t>(name, list){}
	Setting(const char *name, uint8_t defVal, std::vector<ISaveable*> *list) : BaseSetting<uint8_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, uint8_t val) override
	{
		return nvs_set_u8(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, uint8_t *val) override
	{
		return nvs_get_u8(my_handle, name, val);
	}

};

template<>
class Setting<int8_t> : public BaseSetting<int8_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<int8_t>(name, list){}
	Setting(const char *name, int8_t defVal, std::vector<ISaveable*> *list) : BaseSetting<int8_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, int8_t val) override
	{
		return nvs_set_i8(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, int8_t *val) override
	{
		return nvs_get_i8(my_handle, name, val);
	}

};

template<>
class Setting<uint16_t> : public BaseSetting<uint16_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<uint16_t>(name, list){}
	Setting(const char *name, uint16_t defVal, std::vector<ISaveable*> *list) : BaseSetting<uint16_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, uint16_t val) override
	{
		return nvs_set_u16(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, uint16_t *val) override
	{
		return nvs_get_u16(my_handle, name, val);
	}

};

template<>
class Setting<int16_t> : public BaseSetting<int16_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<int16_t>(name, list){}
	Setting(const char *name, int16_t defVal, std::vector<ISaveable*> *list) : BaseSetting<int16_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, int16_t val) override
	{
		return nvs_set_i16(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, int16_t *val) override
	{
		return nvs_get_i16(my_handle, name, val);
	}

};
template<>
class Setting<uint32_t> : public BaseSetting<uint32_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<uint32_t>(name, list){}
	Setting(const char *name, uint32_t defVal, std::vector<ISaveable*> *list) : BaseSetting<uint32_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, uint32_t val) override
	{
		return nvs_set_u32(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, uint32_t *val) override
	{
		return nvs_get_u32(my_handle, name, val);
	}

};

template<>
class Setting<int32_t> : public BaseSetting<int32_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<int32_t>(name, list){}
	Setting(const char *name, int32_t defVal, std::vector<ISaveable*> *list) : BaseSetting<int32_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, int32_t val) override
	{
		return nvs_set_i32(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, int32_t *val) override
	{
		return nvs_get_i32(my_handle, name, val);
	}

};
template<>
class Setting<uint64_t> : public BaseSetting<uint64_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<uint64_t>(name, list){}
	Setting(const char *name, uint64_t defVal, std::vector<ISaveable*> *list) : BaseSetting<uint64_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, uint64_t val) override
	{
		return nvs_set_u64(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, uint64_t *val) override
	{
		return nvs_get_u64(my_handle, name, val);
	}

};

template<>
class Setting<int64_t> : public BaseSetting<int64_t>
{
public:
	Setting(const char *name, std::vector<ISaveable *> *list) : BaseSetting<int64_t>(name, list){}
	Setting(const char *name, int64_t defVal, std::vector<ISaveable*> *list) : BaseSetting<int64_t>(name, defVal, list){}

protected:

	virtual esp_err_t DoSave(nvs_handle_t my_handle, const char *name, int64_t val) override
	{
		return nvs_set_i64(my_handle, name, val);
	}

	virtual esp_err_t DoLoad(nvs_handle_t my_handle, const char *name, int64_t *val) override
	{
		return nvs_get_i64(my_handle, name, val);
	}

};




#endif /* MAIN_SETTINGS_H_ */
