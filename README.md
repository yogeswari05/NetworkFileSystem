# Network File System (NFS) Implementation

This project involves the implementation of a distributed Network File System (NFS) in C, which allows clients to interact with a centralized storage infrastructure using various file-related operations. The NFS comprises multiple components, including clients, storage servers, and a naming server, which together manage and execute operations like reading, writing, creating, deleting, and streaming files over the network.

## Table of Contents
1. [Overview](#overview)
2. [Components](#components)
   - [Clients](#clients)
   - [Naming Server (NM)](#naming-server-nm)
   - [Storage Servers (SS)](#storage-servers-ss)
3. [Features](#features)
   - [File Operations](#file-operations)
   - [Asynchronous and Synchronous Writes](#asynchronous-and-synchronous-writes)
   - [Multiple Clients](#multiple-clients)
   - [Error Handling](#error-handling)
   - [Search Optimization and Caching](#search-optimization-and-caching)
   - [Backup and Redundancy](#backup-and-redundancy)
4. [Installation](#installation)
5. [Running the System](#running-the-system)
6. [Usage](#usage)
7. [Logging and Debugging](#logging-and-debugging)
8. [License](#license)

## Overview
This project simulates a distributed NFS system where multiple clients interact with multiple storage servers through a naming server. The primary functions of the system are to manage file creation, reading, writing, deletion, and streaming over the network. The system is designed to be scalable and fault-tolerant, supporting asynchronous writes, multiple concurrent clients, and storage server replication for backup.

## Components

### Clients
Clients represent the systems or users requesting file access in the NFS. They initiate various file operations (read, write, delete, create, stream) and communicate with the Naming Server (NM) to locate the appropriate Storage Server (SS) where the file is stored.

### Naming Server (NM)
The Naming Server is a central hub in the NFS architecture. It acts as a directory service, mapping client file requests to the appropriate Storage Server. The NM keeps track of all accessible paths in the system and dynamically updates them when new paths are created, deleted, or modified.

### Storage Servers (SS)
Storage Servers store the actual files and provide direct access to clients for file operations. Each SS is responsible for managing a set of files and directories, including their persistence, reading, writing, and streaming. Storage Servers are registered with the NM, which manages their communication with clients.

## Features

### File Operations
Clients can initiate various file operations through the NFS:
- **Create a File/Folder**: Clients can request the creation of a file or folder, and the NM will allocate the task to an available Storage Server.
- **Delete a File/Folder**: Clients can request to delete files or folders. The NM forwards the request to the appropriate SS for deletion.
- **Read a File**: Clients can request to read a file from a Storage Server, which will stream the contents to the client.
- **Write to a File**: Clients can write data to files. This operation supports both synchronous and asynchronous writes.
- **Streaming Audio Files**: Clients can request to stream audio files directly from the SS for playback.

### Asynchronous and Synchronous Writes
- **Asynchronous Writes**: Optimized for large file transfers, allowing clients to continue operations without waiting for the file to be fully written. 
- **Synchronous Writes**: Provides priority for critical operations by waiting until the write is complete.

### Multiple Clients
- **Concurrent Access**: Multiple clients can read the same file simultaneously.
- **Exclusive Writes**: Only one client can write to a file at a time (either synchronously or asynchronously).

### Error Handling
- **Descriptive Error Codes**: Examples include `File Not Found`, `File in Use`, and `Server Failure`.

### Search Optimization and Caching
- **Efficient Search**: Uses hashmaps and tries for fast lookups.
- **LRU Caching**: Caches recent searches to improve response times.

### Backup and Redundancy
- **Replication**: Files are replicated across multiple servers to ensure fault tolerance.
- **Asynchronous Duplication**: Write operations are duplicated across replicated Storage Servers.
- **Server Recovery**: Synchronizes files when a Storage Server comes back online after failure.

## Installation

Clone the repository:
   ```bash
   git clone https://github.com/your-username/nfs.git
   cd nfs
