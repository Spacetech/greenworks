// note: this only contains the typings I needed

// IGreenworks would fit better :)
export interface ISteamworks {
    UGCMatchingType: { Items: number };
    UserUGCListSortOrder: { CreationOrderAsc: number };
    UserUGCList: { Published: number };
    networking: ISteamworksNetworking;
    Utils: {
        createArchive(target: string, src: string, success: () => void, failure: (err: string) => void): void;
        extractArchive(src: string, dest: string): Promise<void>;
    };
    initialize(): boolean;
    shutdown(): void;
    runCallbacks(): void;
    getFriends(): ISteamFriend[];
    getStatInt(name: string): number | undefined;
    setStat(name: string, value: number): void;
    storeStats(cb: (err: string | null) => void): void;
    getGlobalStatInt(name: string, count: number): number | undefined;
    startPlaytimeTracking(publishFileIds: number[]): void;
    stopPlaytimeTracking(): void;
    setRichPresence(key: string, value: string): boolean;
    clearRichPresence(): void;
    createLobby(type: LobbyType): void;
    setLobbyType(lobbyId: string, type: LobbyType): boolean;
    joinLobby(lobbyId: string): void;
    leaveLobby(lobbyId: string): void;
    getLobbyData(lobbyId: string, name: string): string | undefined;
    setLobbyData(lobbyId: string, name: string, value: string): boolean;
    getLobbyOwner(lobbyId: string): string | undefined;
    getLobbyMembers(lobbyId: string): ISteamFriend[] | undefined;
    ugcGetUserItems(type: number, sort: number, listType: number, cb: (err: string | null, items: IWorkshopItem[]) => void): void;
    ugcSynchronizeItems(path: string, cb: (err: string | null, items: IWorkshopItem[]) => void): void;
    ugcUnsubscribe(publishId: string, cb: (err: string | null) => void): void;
    saveFilesToCloud(files: string[], cb: (err: string | null) => void): void;
    publishWorkshopFile(path: string, imagePath: string, title: string, description: string, tags: string[], cb: (err: string | null, publishedFileId2: string) => void): void;
    updatePublishedWorkshopFile(publishFileId: string, path: string, imagePath: string, title: string, description: string, tags: string[], cb: (err: string | null, publishedFileId2: string) => void): void;
    fileShare(path: string, cb: (err: string | null) => void): void;
    activateGameOverlayToWebPage(url: string): void;
    activateGameOverlayInviteDialog(lobbyId: string): void;
    ugcShowOverlay(publishFileId?: string): void;
    getCurrentGameInstallDir(): string;
    getSteamId(): ISteamId;
    getCurrentBetaName(): string;
    onGameOverlayActive(cb: (isActive: boolean) => void): void;
    onLobbyCreated(cb: (success: boolean, lobbyId: string, result?: number) => void): void;
    onLobbyEntered(cb: (success: boolean, lobbyId: string, result?: number) => void): void;
    onLobbyChatUpdate(cb: (lobbyId: string, steamIdUserChanged: string, state: number) => void): void;
    onLobbyJoinRequested(cb: (lobbyId: string | undefined) => void): void;
    getFileCount(): number;
    getFileNameAndSize(index: number): IRemoteFile;
    deleteRemoteFile(name: string): void;
}

export interface ISteamworksNetworking {
    initRelayNetworkAccess(): undefined;
    getRelayNetworkStatus(): ISteamNetworkRelayStatus;

    acceptSessionWithUser(steamIdRemote: string): boolean;
    sendMessageToUser(steamIdRemote: string, data: Uint8Array): number;
    closeSessionWithUser(steamIdRemote: string): boolean;
    getSessionConnectionInfo(steamIdRemote: string): ISteamNetworkSessionConnectionInfo;

    receiveMessagesOnChannel(): Array<{ steamIdRemote: string; data: Uint8Array }> | undefined;

    setSteamNetworkingMessagesSessionRequestCallback(callback: (steamIdRemote: string) => void): void;
    setSteamNetworkingMessagesSessionFailedCallback(callback: (steamIdRemote: string, state: SteamNetworkingConnectionState, endReason: number) => void): void;
    setSteamNetworkingConnectionStatusCallback(callback: (steamIdRemote: string, state: SteamNetworkingConnectionState, endReason: number, oldState: SteamNetworkingConnectionState) => void): void;
    setSteamNetworkingSendRates(min: number, max: number): void;
    setSteamNetworkingDebugCallback(callback: (type: number, message: string) => void): void;

    // ISteamNetworking - this is deprecated. use the above ISteamNetworkingMessages instead
    // acceptP2PSessionWithUser(steamIdRemote: string): boolean;
    // isP2PPacketAvailable(): number;
    // getP2PSessionState(steamIdRemote: string): ISteamNetworkSessionState | undefined;
    // closeP2PSessionWithUser(steamIdRemote: string): boolean;
    // sendP2PPacket(steamIdRemote: string, data: Uint8Array): boolean;
    // readP2PPacket(length: number): { steamIdRemote: string; data: Uint8Array } | undefined;
    // readP2PPacket(length: number, data: Uint8Array): string | undefined;
    // setP2PSessionRequestCallback(callback: (steamIdRemote: string) => void): void;
    // setP2PSessionConnectFailCallback(callback: (steamIdRemote: string, errorCode: number) => void): void;
}

export interface ISteamFriend {
    name?: string;
    steamId: string;
    staticAccountId: string;
    gameId?: string;
    lobbyId?: string;
}

export interface IWorkshopItem {
    file: string;
    fileName: string;
    isUpdated: boolean;
    timeCreated: number;
    timeUpdated: number;
    timeAddedToUserList: number;
    title: string;
    description: string;
    banned: boolean;
    acceptedForUse: boolean;
    publishedFileId: string;
    steamIDOwner: string;
}

export interface ISteamId {
    accountId: number;
    screenName: string;
    steamId: string;
    staticAccountId: string;
}

export interface IRemoteFile {
    name: string;
    size: number;
}

export interface ISteamNetworkRelayStatus {
    availabilitySummary: number;
    availabilityNetworkConfig: number;
    availabilityAnyRelay: number;
    debugMessage: string;
}

export interface ISteamNetworkSessionState {
    connectionActive: number;
    connecting: number;
    lastError: number;
    usingRelay: number;
}

export interface ISteamNetworkSessionConnectionInfo {
    state: SteamNetworkingConnectionState;
    endReason: number;
    connectionDescription: string;
    endDebug: string;
}

export enum SteamNetworkingConnectionState {
    /// Dummy value used to indicate an error condition in the API.
    /// Specified connection doesn't exist or has already been closed.
    k_ESteamNetworkingConnectionState_None = 0,

    /// We are trying to establish whether peers can talk to each other,
    /// whether they WANT to talk to each other, perform basic auth,
    /// and exchange crypt keys.
    ///
    /// - For connections on the "client" side (initiated locally):
    ///   We're in the process of trying to establish a connection.
    ///   Depending on the connection type, we might not know who they are.
    ///   Note that it is not possible to tell if we are waiting on the
    ///   network to complete handshake packets, or for the application layer
    ///   to accept the connection.
    ///
    /// - For connections on the "server" side (accepted through listen socket):
    ///   We have completed some basic handshake and the client has presented
    ///   some proof of identity.  The connection is ready to be accepted
    ///   using AcceptConnection().
    ///
    /// In either case, any unreliable packets sent now are almost certain
    /// to be dropped.  Attempts to receive packets are guaranteed to fail.
    /// You may send messages if the send mode allows for them to be queued.
    /// but if you close the connection before the connection is actually
    /// established, any queued messages will be discarded immediately.
    /// (We will not attempt to flush the queue and confirm delivery to the
    /// remote host, which ordinarily happens when a connection is closed.)
    k_ESteamNetworkingConnectionState_Connecting = 1,

    /// Some connection types use a back channel or trusted 3rd party
    /// for earliest communication.  If the server accepts the connection,
    /// then these connections switch into the rendezvous state.  During this
    /// state, we still have not yet established an end-to-end route (through
    /// the relay network), and so if you send any messages unreliable, they
    /// are going to be discarded.
    k_ESteamNetworkingConnectionState_FindingRoute = 2,

    /// We've received communications from our peer (and we know
    /// who they are) and are all good.  If you close the connection now,
    /// we will make our best effort to flush out any reliable sent data that
    /// has not been acknowledged by the peer.  (But note that this happens
    /// from within the application process, so unlike a TCP connection, you are
    /// not totally handing it off to the operating system to deal with it.)
    k_ESteamNetworkingConnectionState_Connected = 3,

    /// Connection has been closed by our peer, but not closed locally.
    /// The connection still exists from an API perspective.  You must close the
    /// handle to free up resources.  If there are any messages in the inbound queue,
    /// you may retrieve them.  Otherwise, nothing may be done with the connection
    /// except to close it.
    ///
    /// This stats is similar to CLOSE_WAIT in the TCP state machine.
    k_ESteamNetworkingConnectionState_ClosedByPeer = 4,

    /// A disruption in the connection has been detected locally.  (E.g. timeout,
    /// local internet connection disrupted, etc.)
    ///
    /// The connection still exists from an API perspective.  You must close the
    /// handle to free up resources.
    ///
    /// Attempts to send further messages will fail.  Any remaining received messages
    /// in the queue are available.
    k_ESteamNetworkingConnectionState_ProblemDetectedLocally = 5,
}

export enum LobbyType {
    Private = 0,		// only way to join the lobby is to invite to someone else
    FriendsOnly = 1,	// shows for friends or invitees, but not in lobby list
    Public = 2,			// visible for friends and in lobby list
    Invisible = 3,
}
