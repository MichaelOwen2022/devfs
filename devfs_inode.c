/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "devfs.h"

#include "devfs_os.h"
DEVFS_LOG_MODULE_REG(devfs_inode);

struct inode_root_t {
	bool inited;
	struct list_head head;
};

static struct inode_root_t _inode_root;
static struct inode_root_t *inode_root = &_inode_root;

static devfs_mutex_t _inode_mutex;
static devfs_mutex_t *inode_mutex = &_inode_mutex;

static struct devfs_inode_t devfs_inodes[DEVFS_INODE_MAX] = {0};

int devfs_inode_init(void)
{
	if (inode_root->inited == true) {
		return -EACCES;
	}

	devfs_mutex_init(inode_mutex);

	for (int i = 0; i < DEVFS_INODE_MAX; i++) {
		INIT_LIST_HEAD(&(devfs_inodes[i].head));
	}

	INIT_LIST_HEAD(&(inode_root->head));

	inode_root->inited = true;

	DEVFS_DEBUG("%s success", __func__);

	return 0;
}

int devfs_inode_exit(void)
{
	if (inode_root->inited == false) {
		DEVFS_WARN("inode root don't inited");
		return 0;
	}

	inode_root->inited = false;

	devfs_mutex_free(inode_mutex);

	DEVFS_INFO("%s finish", __func__);

	return 0;
}

int devfs_inode_lock(void)
{
	return devfs_mutex_lock(inode_mutex, DEVFS_FOREVER);
}

int devfs_inode_unlock(void)
{
	return devfs_mutex_unlock(inode_mutex);
}

int devfs_inode_malloc(struct devfs_inode_t **inode, const char *name,
					   enum devfs_type_t type, const struct devfs_inode_ops *ops, void *data)
{
	DEVFS_ASSERT(inode);
	DEVFS_ASSERT(name);
	DEVFS_ASSERT(ops);

	if (inode_root->inited == false) {
		DEVFS_ERROR("inode root don't inited");
		return -EACCES;
	}

	*inode = NULL;

	devfs_mutex_lock(inode_mutex, DEVFS_FOREVER);

	for (int i = 0; i < DEVFS_INODE_MAX; i++) {
		struct devfs_inode_t *p = &(devfs_inodes[i]);

		if (!list_empty(&(p->head))) {
			if (strcmp(p->name, name) == 0)
			{
				DEVFS_ERROR("inode name[%s] is exist", name);

				*inode = NULL;
				devfs_mutex_unlock(inode_mutex);

				return -EEXIST;
			}
		} else if (*inode == NULL) {
			DEVFS_DEBUG("find a idle inode %d", i);

			*inode = &devfs_inodes[i];
		}
	}

	if (*inode == NULL) {
		DEVFS_ERROR("idle inode isn't exist");
		devfs_mutex_unlock(inode_mutex);
		return -ENOMEM;
	}

	strncpy((*inode)->name, name, DEVFS_NAME_MAX);
	(*inode)->name[DEVFS_NAME_MAX] = '\0';

	(*inode)->type = type;
	(*inode)->dev_ops = ops;
	(*inode)->dev_data = data;
	(*inode)->references = 0;

	list_add_tail(&((*inode)->head), &(inode_root->head));

	devfs_mutex_unlock(inode_mutex);

	return 0;
}

int devfs_inode_free(struct devfs_inode_t *inode)
{
	DEVFS_ASSERT(inode);

	if (inode_root->inited == false) {
		DEVFS_ERROR("inode root don't inited");
		return -EACCES;
	}

	devfs_mutex_lock(inode_mutex, DEVFS_FOREVER);

	/* Can't free if inode is opened */
	if (inode->references > 0) {
		devfs_mutex_unlock(inode_mutex);
		return -EACCES;
	}

	list_del_init(&(inode->head));

	devfs_mutex_unlock(inode_mutex);

	return 0;
}

int devfs_inode_search(struct devfs_inode_t **inode, const char *name)
{
	struct list_head *head = NULL;

	DEVFS_ASSERT(inode);
	DEVFS_ASSERT(name);

	if (inode_root->inited == false) {
		DEVFS_ERROR("inode root don't inited");
		return -EACCES;
	}

	*inode = NULL;

	devfs_mutex_lock(inode_mutex, DEVFS_FOREVER);

	list_for_each(head, &(inode_root->head)) {
		*inode = (struct devfs_inode_t *)head;

		if (strcmp((*inode)->name, name) == 0) {
			devfs_mutex_unlock(inode_mutex);
			return 0;
		}
	}

	devfs_mutex_unlock(inode_mutex);

	return -ENOENT;
}

int devfs_inode_search_with_type(struct devfs_inode_t **inode, const char *name, enum devfs_type_t type)
{
	struct list_head *head = NULL;

	DEVFS_ASSERT(inode);
	DEVFS_ASSERT(name);

	if (inode_root->inited == false) {
		DEVFS_ERROR("inode root don't inited");
		return -EACCES;
	}

	*inode = NULL;

	devfs_mutex_lock(inode_mutex, DEVFS_FOREVER);

	list_for_each(head, &(inode_root->head)) {
		*inode = (struct devfs_inode_t *)head;

		if (strcmp((*inode)->name, name) == 0 &&
			(*inode)->type == type) {
			devfs_mutex_unlock(inode_mutex);
			return 0;
		}
	}

	devfs_mutex_unlock(inode_mutex);

	return -ENOENT;
}

int devfs_inode_first(struct devfs_inode_t **inode)
{
	struct list_head *head = NULL;

	DEVFS_ASSERT(inode);

	if (inode_root->inited == false) {
		DEVFS_ERROR("inode root don't inited");
		return -EACCES;
	}

	*inode = NULL;

	devfs_mutex_lock(inode_mutex, DEVFS_FOREVER);

	if (!list_empty(&(inode_root->head))) {
		head = inode_root->head.next;
		*inode = (struct devfs_inode_t *)head;
	}

	devfs_mutex_unlock(inode_mutex);

	return 0;
}

int devfs_inode_next(struct devfs_inode_t **inode)
{
	struct list_head *head = NULL;

	DEVFS_ASSERT(inode);

	if (inode_root->inited == false) {
		DEVFS_ERROR("inode root don't inited");
		return -EACCES;
	}

	if (*inode == NULL) {
		return -ENOENT;
	}

	devfs_mutex_lock(inode_mutex, DEVFS_FOREVER);

	head = (*inode)->head.next;

	if (head == &(inode_root->head)) {
		*inode = NULL;
	} else {
		*inode = (struct devfs_inode_t *)head;
	}

	devfs_mutex_unlock(inode_mutex);

	return 0;
}
